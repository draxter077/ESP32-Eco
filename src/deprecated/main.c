// =============================================================================
// main.c  -  "Quem roda e quando?"
//
// Liga os módulos e cria as 3 tarefas. Não tem lógica de decisão: quem decide
// é o control.c.
//
//   task_measure   (20 ms)   mede, integra Ah/Wh e confere a segurança
//   task_control   (100 ms)  executa a máquina de estados e resistores
//   task_interface (500 ms)  display, site e log
//
// =============================================================================


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "nvs_flash.h"


#include "config.h"
#include "hardware.h"
#include "control.h"
#include "logger.h"
#include "network.h"
#include "display.h"


static const char *TAG = "main";


#define APP_CORE    1       // tarefas críticas no núcleo 1 (o Wi-Fi usa o núcleo 0)




// task_measure: lê os sensores e confere a segurança (prioridade mais alta)
// Executa a cada 20ms
static void task_measure(void *arg)
{
   esp_task_wdt_add(NULL);             


   TickType_t last_wake = xTaskGetTickCount();
   int64_t last_us = esp_timer_get_time();
   int fail_count = 0;


   while (1) {
       float v = 0, i = 0, t = 0;
       bool ok = read_voltage(&v) && read_current(&i) && read_temperature(&t);


       int64_t now_us = esp_timer_get_time();
       float dt_s = (float)(now_us - last_us) / 1e6f;
       last_us = now_us;


       if (!ok) {
           // Tolera falhas isoladas; só dispara depois de várias seguidas
           if (++fail_count >= SENSOR_FAIL_LIMIT) {
               enter_error(ERR_SENSOR);
           }
       } else {
           fail_count = 0;
           control_update_measurements(v, i, t, dt_s);


           error_code_t err = check_safety(v, i, t);
           if (err != ERR_NONE) {
               enter_error(err);
           }
       }


       esp_task_wdt_reset();
       vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(MEASURE_PERIOD_MS));
   }
}




// task_control: máquina de estados e banco de resistores
// Executa a cada 100ms
static void task_control(void *arg)
{
   esp_task_wdt_add(NULL);


   TickType_t last_wake = xTaskGetTickCount();
   while (1) {
       control_step();
       esp_task_wdt_reset();
       vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
   }
}




// task_interface: só lê a telemetria e mostra/grava (não aciona relés / mosfets)
// Executa a cada 500ms
static void task_interface(void *arg)
{
   TickType_t last_wake = xTaskGetTickCount();
   bool led = false;
   int log_count = 0;
   const int log_every = LOG_PERIOD_MS / INTERFACE_PERIOD_MS;


   while (1) {
       telemetry_t t;
       control_get_telemetry(&t);


       // Display
       if (t.state == STATE_ERROR) {
           display_show_error(t.error);
       } else {
           display_update(&t);
       }


       // Site
       network_send_telemetry(&t);


       // LED de erro pisca; nos outros estados fica apagado
       if (t.state == STATE_ERROR) {
           led = !led;
           led_set(led);
       } else {
           led_set(false);
       }


       // Log: 1 amostra por LOG_PERIOD_MS, só com processo em andamento
       if (t.state == STATE_CHARGING || t.state == STATE_DISCHARGING) {
           if (log_count % log_every == 0) {
               logger_add_sample(&t);
           }
           log_count++;
       } else {
           log_count = 0;
       }


       vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(INTERFACE_PERIOD_MS));
   }
}




// app_main: ordem de inicialização importa
// Função principal
void app_main(void)
{
   // Inicia o hardware e relés/mosfets
   hardware_init();


   // Reinicia tudo para começar sempre no mesmo ponto
   esp_register_shutdown_handler(all_off);


   // Inicia o controle
   control_init();


   // Inicialisa o flash
   esp_err_t err = nvs_flash_init();
   if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
       ESP_ERROR_CHECK(nvs_flash_erase());
       err = nvs_flash_init();
   }
   ESP_ERROR_CHECK(err);


   // Inicia a interface / display
   if (logger_init() != ESP_OK) {
       ESP_LOGW(TAG, "Logger indisponivel: sem gravacao de CSV");
   }
   if (display_init() != ESP_OK) {
       ESP_LOGW(TAG, "Display indisponivel");
   }
   if (network_init() != ESP_OK) {
       ESP_LOGW(TAG, "Rede indisponivel: so controle local");
   }


   // Tarefas
   xTaskCreatePinnedToCore(task_measure,   "measure",   TASK_MEASURE_STACK,   NULL,
                           TASK_MEASURE_PRIO,   NULL, APP_CORE);
   xTaskCreatePinnedToCore(task_control,   "control",   TASK_CONTROL_STACK,   NULL,
                           TASK_CONTROL_PRIO,   NULL, APP_CORE);
   xTaskCreate(task_interface, "interface", TASK_INTERFACE_STACK, NULL,
               TASK_INTERFACE_PRIO, NULL);


   ESP_LOGI(TAG, "Sistema iniciado");
}
