/*
O que aparece na tela?
*/

#pragma once

#include "config.h"
#include "esp_err.h"

// Inicializa o display
esp_err_t display_init(void);

// Desenha a tela de status
// Chamada por task_interface a cada 500 ms
// ******** Avaliar o que estará no display
void display_update(const telemetry_t *t);

// Mostra o código do erro em destaque
// O erro também reflete no site e no LED
void display_show_error(error_code_t code);

// Mensagem curta de uma linha
void display_show_message(const char *text);
