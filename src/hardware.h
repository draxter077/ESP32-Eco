/*
Como falo com o mundo físico?
*/

#pragma once

#include "config.h"
#include <stdbool.h>

// Identificadores dos relés
typedef enum {
  RELAY_1 = 0, // bateria -> circuito de descarga
  RELAY_2,     // fonte de carga -> bateria
  RELAY_3,     // resistor de 4,5 ohm
  RELAY_4,     // resistor de 18 ohm
  RELAY_5,     // reserva
} relay_id_t;

// Identificadores dos botões de comando
// ***** Avaliar quais botões serão *************
typedef enum { BTN_CHARGE = 0, BTN_DISCHARGE } button_id_t;

// ----- Inicialização ---------------------------------------------------------

// Primeiro coloca relés e ventoinha em estado desligado, depois configura os
// sensores e display Chamar antes de qualquer outra coisa no main()
void hardware_init(void);

// ----- Sensores (retornam false se a leitura falhar) ----------

// Lê a tensão
bool read_voltage(float *volts);

// Lê a corrente
// Positivo p/ carga e negativo p/ descarga
bool read_current(float *amps);

// Lê a temperatura (falso se o sensor estiver aberto ou em curto)
bool read_temperature(float *celsius);

// Tensão final da descarga ajustada no potenciômetro
// já limitada entre DISCHARGE_MIN_FINAL_V e DISCHARGE_MAX_FINAL_V
bool read_final_voltage_pot(float *volts);

// Mede o offset da corrente com tudo desligado e guarda para as próximas
// leituras
bool calibrate_current_zero(void);

// ----- Saídas ----------------------------------------------------------------

// Liga ou desliga um relé
void relay_set(relay_id_t id, bool on);

// Estado atual comandado do relé
bool relay_get(relay_id_t id);

// Desliga os relés e a ventoinha
void all_off(void);

// Liga ou desliga a ventoinha
void fan_set(bool on);

// Liga ou desliga o LED de erro
void led_set(bool on);

// ----- Entradas --------------------------------------------------------------

// Identifica o botão apertado uma vez a cada aperto
bool button_pressed(button_id_t id);

// Estado de emergência (botão ou falha no sistema)
bool emergency_active(void);
