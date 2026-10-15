/*
Quais são os números do projeto?
*/

#pragma once

#include "driver/gpio.h"
#include <stdbool.h>
#include <stdint.h>

// 1. PACK DE BATERIAS

// ************* Ver valores certinho **********
#define CELLS_SERIES 3     // Numero de celulas em serie
#define CELL_V_FULL 4.2f   // V por célula, carga completa
#define CELL_V_CUTOFF 3.0f // V por célula, corte padrão da descarga
#define CELL_V_MIN 2.5f    // V por célula, mínimo absoluto

#define PACK_V_FULL (CELL_V_FULL * CELLS_SERIES)
#define PACK_V_CUTOFF (CELL_V_CUTOFF * CELLS_SERIES)

// 2. PINOS

// Relés
//   RL1 = liga a bateria ao circuito de descarga
//   RL2 = liga na fonte de carga da bateria
//   RL3 = resistor de 4,5 ohm
//   RL4 = resistor de 18 ohm
//   RL5 = reserva

// ******** Confirmar no esquematico **********

#define RL1_GPIO GPIO_NUM_26
#define RL2_GPIO GPIO_NUM_27
#define RL3_GPIO GPIO_NUM_25
#define RL4_GPIO GPIO_NUM_33
#define RL5_GPIO GPIO_NUM_32
#define FAN_GPIO GPIO_NUM_23
#define LED_ERROR_GPIO GPIO_NUM_13

// Nível que LIGA o relé: 1 = módulo ativo em nível alto, 0 = ativo em nível
// baixo
#define RELAY_ACTIVE_LEVEL 1

// Botões
// ****** Conferir depois *************
#define EMERGENCY_BTN_GPIO GPIO_NUM_4
#define CHARGE_BTN_GPIO GPIO_NUM_16
#define DISCHARGE_BTN_GPIO GPIO_NUM_17
#define BTN_DEBOUNCE_MS 30

// 3. SENSORES

// ******* Faltam Fazer outros sensores ********

// **************** Avaliar valores *******

// Ventoinha: liga durante a descarga e enquanto estiver quente
#define FAN_ON_TEMP_C 40.0f
#define FAN_OFF_TEMP_C 35.0f

// Potenciômetro marca a tensão final na descarga
#define DISCHARGE_MIN_FINAL_V 9.0f
#define DISCHARGE_MAX_FINAL_V 11.5f

-
// 4. SEGURANÇA

#define MAX_VOLTAGE 13.0f
#define MIN_VOLTAGE 7.5f
#define MAX_CURRENT 3.5f
#define MAX_TEMP 60.0f
#define SAFETY_CONFIRM_SAMPLES 3

// 5. CARGA

#define CHARGE_DONE_VOLTAGE 12.5f
#define CHARGE_CUTOFF_CURRENT 0.03f
#define CHARGE_CUTOFF_TIME_S 5
#define CHARGE_MAX_TIME_S (8 * 3600)

// 6. DESCARGA

#define BANK_V_HIGH_ENTER 11.0f // V >= 11,0: só 4,5 ohm
#define BANK_V_LOW_ENTER 10.8f  // V <  10,8: 4,5 ohm paralelo 18 ohm = 3,6 ohm
#define BANK_MIN_SWITCH_MS 5000 // tempo mínimo entre trocas de degrau
#define DISCHARGE_CONFIRM_SAMPLES 5
#define DISCHARGE_MAX_TIME_S (8 * 3600)

// 7. TAREFAS (em ms)

#define MEASURE_PERIOD_MS 20
#define CONTROL_PERIOD_MS 100
#define INTERFACE_PERIOD_MS 500

#define TASK_MEASURE_STACK 4096
#define TASK_CONTROL_STACK 4096
#define TASK_INTERFACE_STACK 6144

#define TASK_MEASURE_PRIO 6 // maior número = maior prioridade
#define TASK_CONTROL_PRIO 5
#define TASK_INTERFACE_PRIO 3

// 8. REDE

#define WIFI_STA_SSID "" // vazio = vai direto para o AP
#define WIFI_STA_PASSWORD ""
#define WIFI_STA_TIMEOUT_MS 10000
#define WIFI_AP_SSID "Carregador de Bateria"
#define WIFI_AP_PASSWORD "123456789"
#define MDNS_HOSTNAME "bateria" // acesso: http://bateria.local

// 9. LOG E DISPLAY

#define LOG_MOUNT_POINT "/littlefs"
#define LOG_FILE_PATH "/littlefs/sessao.csv"
#define LOG_PERIOD_MS 1000
#define LOG_MIN_FREE_BYTES (100 * 1024) // não inicia a sessão com menos espaço
#define LOG_CSV_HEADER "tempo_s,estado,tensao_V,corrente_A,temp_C\n"

#define DISPLAY_I2C_ADDR 0x3C
#define DISPLAY_WIDTH 128 // ******* Ajustar numeros depois ******
#define DISPLAY_HEIGHT 64

    // TIPOS E STRUCTS

    // Estados da máquina de estados (control.c)
    typedef enum {
      STATE_IDLE = 0,    // parado, esperando comando
      STATE_CHARGING,    // carregando
      STATE_DISCHARGING, // descarregando
      STATE_FINISHED,    // terminou, CSV disponível
      STATE_ERROR        // proteção disparada, tudo desligado
    } state_t;

// Erros
typedef enum {
  ERR_NONE = 0,
  ERR_EMERGENCY,
  ERR_OVERVOLTAGE,
  ERR_UNDERVOLTAGE,
  ERR_OVERCURRENT,
  ERR_OVERTEMP,
  ERR_SENSOR,
  ERR_TIMEOUT,
  ERR_INTERLOCK,
  ERR_STORAGE
} error_code_t;

// Comandos de botões e site
typedef enum {
  CMD_NONE = 0,
  CMD_START_CHARGE,
  CMD_START_DISCHARGE,
  CMD_STOP,
  CMD_RESET_ERROR
} command_t;

// Dados de telemetria
// Quem mede escreve, quem mostra apenas lê
typedef struct {
  float voltage;
  float current;
  float temp;
  // Avaliando a necessidade
  float ah;
  float wh;

  float final_voltage;
  uint32_t time_s;
  state_t state;
  error_code_t error;
} telemetry_t;
