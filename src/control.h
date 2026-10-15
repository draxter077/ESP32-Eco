/*
O que o sistema deve fazer agora?
*/

#pragma once

#include "config.h"
#include <stdbool.h>

// Inicia a telemetria, zera os dados e deixa tudo desligado (STATE_IDLE)
void control_init(void);

// Chamada por task_measure (20 ms): guarda V, I e T na telemetria e
// integra Ah e Wh usando dt_s (tempo desde a última chamada, em segundos)
void control_update_measurements(float volts, float amps, float celsius,
                                 float dt_s);

// Chamada por task_control (100 ms): lê os botões, executa o switch(state),
// atualiza o banco de resistores e verifica o fim do processo
void control_step(void);

// ----- Segurança
// --------------------------------------------------------------

// Confere os limites. Retorna ERR_NONE se estiver tudo bem, ou o código do
// erro. Considera também a emergência e o tempo máximo do processo.
error_code_t check_safety(float volts, float amps, float celsius);

// Desliga tudo (all_off), vai para STATE_ERROR e registra o código
void enter_error(error_code_t code);

// ----- Pedidos (botões e site) -----------------------------------------------

// Pede uma ação. Retorna false se o pedido for recusado
// (ex.: já existe processo ativo, emergência ativa, tensão final inválida).
// final_voltage só é usado em CMD_START_DISCHARGE.
// Seguro de chamar de outra tarefa (ex.: handler HTTP).
bool control_request(command_t cmd, float final_voltage);

// ----- Leitura dos dados -----------------------------------------------------

// Copia a telemetria atual (protegida por mutex). Usada por logger, display e
// network.
void control_get_telemetry(telemetry_t *out);

// ----- Textos para tela e site -----------------------------------------------
const char *state_to_text(state_t state);
const char *error_to_text(error_code_t code);
