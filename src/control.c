// =============================================================================
// control.c  -  "O que o sistema deve fazer agora?"
//
// É o ÚNICO arquivo que decide ligar ou desligar os relés.
//
// Quem mexe em quê:
//   task_measure (20 ms)  -> control_update_measurements(), check_safety(), enter_error()
//   task_control (100 ms) -> control_step()
//   site / botões         -> control_request()  (só PEDE; control_step executa)
//
// Estados:  IDLE -> CHARGING / DISCHARGING -> FINISHED
//           qualquer estado -> ERROR
// =============================================================================
#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "config.h"
#include "control.h"
#include "hardware.h"
#include "logger.h"

static const char *TAG = "control";

#define CMD_QUEUE_LENGTH    4

#define LOCK()      xSemaphoreTake(s_mutex, portMAX_DELAY)
#define UNLOCK()    xSemaphoreGive(s_mutex)


// Tipos e variáveis internas

typedef struct {
    command_t cmd;
    float     final_voltage;
} cmd_msg_t;

typedef enum {
    BANK_OFF = 0,       // nenhum resistor ligado
    BANK_HIGH,          // só RL3 (4,5 ohm)
    BANK_LOW            // RL3 + RL4 (4,5 ohm // 18 ohm = 3,6 ohm)
} bank_step_t;

static SemaphoreHandle_t s_mutex = NULL;    // protege s_tel
static QueueHandle_t     s_queue = NULL;    // pedidos vindos de botões e site
static telemetry_t       s_tel;             // dados compartilhados

static double   s_ah_acc = 0.0;             // acumuladores em double (evita perder
static double   s_wh_acc = 0.0;             // precisão ao somar valores pequenos)

static volatile bool s_error_latched = false;   // true enquanto estiver em ERROR
static bool     s_logger_open  = false;     // só usado pela task_control
static int64_t  s_start_us     = 0;         // instante em que o processo começou
static int      s_end_count    = 0;         // amostras seguidas na condição de fim

static bank_step_t s_bank_step    = BANK_OFF;
static int64_t     s_bank_last_ms = 0;      // instante da última troca de degrau

static bool     s_fan_hot = false;          // histerese da ventoinha por temperatura

// Filtro de confirmação do check_safety (só a task_measure usa)
static volatile int s_bad_count = 0;
static error_code_t s_bad_code  = ERR_NONE;


// Funções auxiliares

// Retorna o tempo do processo
static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

// Retorna se está no estagio de carregamento ou descarregamento
static bool state_is_active(state_t s)
{
    return (s == STATE_CHARGING) || (s == STATE_DISCHARGING);
}

// Desliga os relés 
static void relays_off(void)
{
    for (int r = 0; r < RELAY_COUNT; r++) {
        relay_set((relay_id_t)r, false);
    }
    s_bank_step = BANK_OFF;
}

// Liga um relé
static void relay_on_guarded(relay_id_t id)
{
    if (!s_error_latched) {
        relay_set(id, true);
    }
}

// Muda o estado (não muda estado de ERROR)
// Retorna false se a mudança foi recusada.
static bool set_state(state_t new_state)
{
    bool ok = true;
    LOCK();
    if (s_tel.state == STATE_ERROR) {
        ok = false;
    } else {
        s_tel.state = new_state;
    }
    UNLOCK();
    return ok;
}

// Retorna o tempo que a operação demorou
static void update_elapsed(void)
{
    uint32_t s = (uint32_t)((esp_timer_get_time() - s_start_us) / 1000000);
    LOCK();
    s_tel.elapsed_s = s;
    UNLOCK();
}

// Fecha o canal de salvamento de dados
static void close_logger(void)
{
    if (s_logger_open) {
        logger_stop();
        s_logger_open = false;
    }
}

// Ventoinha: ligada durante a descarga e enquanto estiver quente 
static void update_fan(const telemetry_t *t)
{
    if (t->temp >= FAN_ON_TEMP_C) {
        s_fan_hot = true;
    } else if (t->temp <= FAN_OFF_TEMP_C) {
        s_fan_hot = false;
    }
    fan_set((t->state == STATE_DISCHARGING) || s_fan_hot);
}


// Banco de resistores (descarga)


// RL3 fica ligado nos dois degraus; só o RL4 muda. Assim a carga nunca fica aberta.
// Liga ou desliga os relés / mosfets
static void apply_bank_step(bank_step_t step)
{
    switch (step) {
    case BANK_HIGH:
        relay_on_guarded(RELAY_3);
        relay_set(RELAY_4, false);
        break;
    case BANK_LOW:
        relay_on_guarded(RELAY_3);
        relay_on_guarded(RELAY_4);
        break;
    default:
        relay_set(RELAY_3, false);
        relay_set(RELAY_4, false);
        break;
    }
    s_bank_step    = step;
    s_bank_last_ms = now_ms();
}

// Escolhe o degrau de tensão
static void update_resistor_bank(float voltage)
{
    if (s_bank_step == BANK_OFF) {
        return;
    }
    if ((now_ms() - s_bank_last_ms) < BANK_MIN_SWITCH_MS) {
        return;
    }
    if (s_bank_step == BANK_HIGH && voltage < BANK_V_LOW_ENTER) {
        apply_bank_step(BANK_LOW);
    } else if (s_bank_step == BANK_LOW && voltage >= BANK_V_HIGH_ENTER) {
        apply_bank_step(BANK_HIGH);
    }
}


// Início e fim de processo


// Confere se um pedido pode ser aceito agora
static bool request_is_valid(command_t cmd, float final_voltage)
{
    telemetry_t t;
    control_get_telemetry(&t);

    bool idle_like = (t.state == STATE_IDLE) || (t.state == STATE_FINISHED);
    bool voltage_ok = (t.voltage >= MIN_VOLTAGE) && (t.voltage <= MAX_VOLTAGE);

    switch (cmd) {
    case CMD_START_CHARGE:
        return idle_like && !emergency_active() && voltage_ok;

    case CMD_START_DISCHARGE:
        return idle_like && !emergency_active() && voltage_ok
            && final_voltage >= DISCHARGE_MIN_FINAL_V
            && final_voltage <= DISCHARGE_MAX_FINAL_V
            && t.voltage > final_voltage;      

    case CMD_STOP:
        return state_is_active(t.state);

    case CMD_RESET_ERROR:
        return (t.state == STATE_ERROR) && !emergency_active();

    default:
        return false;
    }
}

// Abre o log e zera os contadores. Retorna false se não puder começar.
static bool begin_session(float final_voltage)
{
    esp_err_t err = logger_start();
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Download do CSV em andamento, tente de novo em instantes");
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao abrir o log (%s)", esp_err_to_name(err));
        enter_error(ERR_STORAGE);
        return false;
    }
    s_logger_open = true;
    s_start_us    = esp_timer_get_time();
    s_end_count   = 0;
    s_ah_acc      = 0.0;
    s_wh_acc      = 0.0;

    LOCK();
    s_tel.ah            = 0.0f;
    s_tel.wh            = 0.0f;
    s_tel.elapsed_s     = 0;
    s_tel.final_voltage = final_voltage;
    UNLOCK();
    return true;
}

// Manda iniciar a carga
static void start_charge(void)
{
    // Intertravamento: nada da descarga pode estar ligado
    if (relay_get(RELAY_1) || relay_get(RELAY_3) ||
        relay_get(RELAY_4) || relay_get(RELAY_5)) {
        enter_error(ERR_INTERLOCK);
        return;
    }
    if (!begin_session(PACK_V_FULL)) {
        return;
    }
    if (!set_state(STATE_CHARGING)) {
        return;                         
    }
    relay_on_guarded(RELAY_2);
    ESP_LOGI(TAG, "Carga iniciada");
}

// Manda iniciar a descarga
static void start_discharge(float final_voltage)
{
    // Intertravamento: a fonte de carga não pode estar ligada
    if (relay_get(RELAY_2)) {
        enter_error(ERR_INTERLOCK);
        return;
    }
    if (!begin_session(final_voltage)) {
        return;
    }
    if (!set_state(STATE_DISCHARGING)) {
        return;
    }

    telemetry_t t;
    control_get_telemetry(&t);

    fan_set(true);
    apply_bank_step(t.voltage < BANK_V_LOW_ENTER ? BANK_LOW : BANK_HIGH);
    relay_on_guarded(RELAY_1);
    ESP_LOGI(TAG, "Descarga iniciada, tensao final %.2f V", final_voltage);
}

// Encerra o processo atual
static void stop_process(void)
{
    all_off();                          
    s_bank_step = BANK_OFF;
    close_logger();
    set_state(STATE_FINISHED);
    ESP_LOGI(TAG, "Processo finalizado");
}

// Limpa o erro e volta para IDLE
static void reset_error(void)
{
    relays_off();
    s_bad_count = 0;
    s_bad_code  = ERR_NONE;
    LOCK();
    s_tel.state = STATE_IDLE;
    s_tel.error = ERR_NONE;
    UNLOCK();
    s_error_latched = false;
    ESP_LOGI(TAG, "Erro limpo, sistema em IDLE");
}

// Retorna true quando o processo atingiu o fim 
static bool check_end_of_process(const telemetry_t *t)
{
    bool condition;
    int  needed;

    if (t->state == STATE_CHARGING) {
        condition = (t->voltage >= CHARGE_DONE_VOLTAGE) &&
                    (fabsf(t->current) < CHARGE_CUTOFF_CURRENT);
        needed = (CHARGE_CUTOFF_TIME_S * 1000) / CONTROL_PERIOD_MS;
    } else {
        condition = (t->voltage <= t->final_voltage);
        needed = DISCHARGE_CONFIRM_SAMPLES;
    }

    s_end_count = condition ? (s_end_count + 1) : 0;
    return s_end_count >= needed;
}


// Botões e fila de pedidos

// Regra dos botões:
//   parado  -> inicia (carga ou descarga, esta com a tensão final do potenciômetro)
//   rodando -> qualquer botão PARA o processo
//   erro    -> qualquer botão limpa o erro (com a emergência liberada)

// Lê os botões
static void handle_buttons(void)
{
    bool charge    = button_pressed(BTN_CHARGE);       // sempre lê, para limpar a borda
    bool discharge = button_pressed(BTN_DISCHARGE);

    if (!charge && !discharge) {
        return;
    }
    if (charge && discharge) {
        return;                         // dois botões juntos: ignora
    }

    telemetry_t t;
    control_get_telemetry(&t);

    if (state_is_active(t.state)) {
        control_request(CMD_STOP, 0.0f);
    } else if (t.state == STATE_ERROR) {
        control_request(CMD_RESET_ERROR, 0.0f);
    } else if (charge) {
        control_request(CMD_START_CHARGE, 0.0f);
    } else {
        float final_v;
        if (read_final_voltage_pot(&final_v)) {
            control_request(CMD_START_DISCHARGE, final_v);
        } else {
            ESP_LOGW(TAG, "Falha ao ler o potenciometro");
        }
    }
}

// Processa o input dos botões
static void process_commands(void)
{
    cmd_msg_t msg;
    while (xQueueReceive(s_queue, &msg, 0) == pdTRUE) {
        if (!request_is_valid(msg.cmd, msg.final_voltage)) {
            continue;                   // o estado mudou desde o pedido
        }
        switch (msg.cmd) {
        case CMD_START_CHARGE:      start_charge();                     break;
        case CMD_START_DISCHARGE:   start_discharge(msg.final_voltage); break;
        case CMD_STOP:              stop_process();                     break;
        case CMD_RESET_ERROR:       reset_error();                      break;
        default:                                                        break;
        }
    }
}


// API pública

// Cria a lista e mutex dos comandos
void control_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    s_queue = xQueueCreate(CMD_QUEUE_LENGTH, sizeof(cmd_msg_t));
    configASSERT(s_mutex != NULL && s_queue != NULL);

    memset(&s_tel, 0, sizeof(s_tel));
    s_tel.state         = STATE_IDLE;
    s_tel.error         = ERR_NONE;
    s_tel.final_voltage = DISCHARGE_MIN_FINAL_V;

    all_off();                          // garantia extra: tudo desligado
}

// Atualiza os sinais de contrle
void control_update_measurements(float volts, float amps, float celsius, float dt_s)
{
    LOCK();
    s_tel.voltage = volts;
    s_tel.current = amps;
    s_tel.temp    = celsius;

    if (state_is_active(s_tel.state) && dt_s > 0.0f) {
        s_ah_acc += (double)fabsf(amps) * dt_s / 3600.0;
        s_wh_acc += (double)fabsf(amps) * volts * dt_s / 3600.0;
        s_tel.ah  = (float)s_ah_acc;
        s_tel.wh  = (float)s_wh_acc;
    }
    UNLOCK();
}

// Coração do controle
// Controla tudo com base em qual estado o sistema está
void control_step(void)
{
    handle_buttons();
    process_commands();

    telemetry_t t;
    control_get_telemetry(&t);

    switch (t.state) {
    case STATE_CHARGING:
        update_elapsed();
        if (relay_get(RELAY_1)) {                   
            enter_error(ERR_INTERLOCK);
            break;
        }
        if (check_end_of_process(&t)) {
            stop_process();
        }
        break;

    case STATE_DISCHARGING:
        update_elapsed();
        if (relay_get(RELAY_2)) {                  
            enter_error(ERR_INTERLOCK);
            break;
        }
        update_resistor_bank(t.voltage);
        if (check_end_of_process(&t)) {
            stop_process();
        }
        break;

    case STATE_ERROR:
        relays_off();                               
        close_logger();
        break;

    case STATE_IDLE:
    case STATE_FINISHED:
    default:
        break;
    }

    control_get_telemetry(&t);                  
    update_fan(&t);
}

// Checa a segurança do sistema
error_code_t check_safety(float volts, float amps, float celsius)
{
    // Emergência vale na hora, sem confirmação
    if (emergency_active()) {
        s_bad_count = 0;
        return ERR_EMERGENCY;
    }

    telemetry_t t;
    control_get_telemetry(&t);

    error_code_t candidate = ERR_NONE;

    if (volts > MAX_VOLTAGE) {
        candidate = ERR_OVERVOLTAGE;
    } else if (fabsf(amps) > MAX_CURRENT) {
        candidate = ERR_OVERCURRENT;
    } else if (celsius > MAX_TEMP) {
        candidate = ERR_OVERTEMP;
    } else if (state_is_active(t.state) && volts < MIN_VOLTAGE) {
        candidate = ERR_UNDERVOLTAGE;
    } else if (t.state == STATE_CHARGING && t.elapsed_s > CHARGE_MAX_TIME_S) {
        candidate = ERR_TIMEOUT;
    } else if (t.state == STATE_DISCHARGING && t.elapsed_s > DISCHARGE_MAX_TIME_S) {
        candidate = ERR_TIMEOUT;
    }

    if (candidate == ERR_NONE) {
        s_bad_count = 0;
        s_bad_code  = ERR_NONE;
        return ERR_NONE;
    }
    if (candidate == s_bad_code) {
        s_bad_count++;
    } else {
        s_bad_code  = candidate;
        s_bad_count = 1;
    }
    return (s_bad_count >= SAFETY_CONFIRM_SAMPLES) ? candidate : ERR_NONE;
}

// Coloca o sistema em estado de ERRO
void enter_error(error_code_t code)
{
    if (s_error_latched) {
        return;                    
    }
    s_error_latched = true;
    all_off();                        

    LOCK();
    s_tel.state = STATE_ERROR;
    s_tel.error = code;
    UNLOCK();

    ESP_LOGE(TAG, "ERRO: %s", error_to_text(code));
}

// Coloca uma ação na lista de controle
bool control_request(command_t cmd, float final_voltage)
{
    if (s_queue == NULL || !request_is_valid(cmd, final_voltage)) {
        return false;
    }
    cmd_msg_t msg = { .cmd = cmd, .final_voltage = final_voltage };
    return xQueueSend(s_queue, &msg, 0) == pdTRUE;
}

// Salva a telemetria no instante
void control_get_telemetry(telemetry_t *out)
{
    LOCK();
    *out = s_tel;
    UNLOCK();
}

// Textos (sem acento, para o display e o CSV)

// Traduz o estado para o portugues
const char *state_to_text(state_t state)
{
    switch (state) {
    case STATE_IDLE:        return "PARADO";
    case STATE_CHARGING:    return "CARGA";
    case STATE_DISCHARGING: return "DESCARGA";
    case STATE_FINISHED:    return "FINALIZADO";
    case STATE_ERROR:       return "ERRO";
    default:                return "?";
    }
}

// Traduz o erro para portugues
const char *error_to_text(error_code_t code)
{
    switch (code) {
    case ERR_NONE:          return "Sem erro";
    case ERR_EMERGENCY:     return "Emergencia";
    case ERR_OVERVOLTAGE:   return "Sobretensao";
    case ERR_UNDERVOLTAGE:  return "Subtensao";
    case ERR_OVERCURRENT:   return "Sobrecorrente";
    case ERR_OVERTEMP:      return "Sobretemperatura";
    case ERR_SENSOR:        return "Falha de sensor";
    case ERR_TIMEOUT:       return "Tempo maximo";
    case ERR_INTERLOCK:     return "Intertravamento";
    case ERR_STORAGE:       return "Armazenamento";
    default:                return "Desconhecido";
    }
}