/*
Como guardo os dados?
*/

#pragma once

#include "config.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include <stdbool.h>

// Inicia o logger
esp_err_t logger_init(void);

// Abre uma nova sessão
// Falha se houver menos que LOG_MIN_FREE_BYTES livres.
esp_err_t logger_start(void);

// Fecha o arquivo da sessão
// Os dados continuam disponíveis para download.
void logger_stop(void);

// Acrescenta uma linha ao arquivo
void logger_add_sample(const telemetry_t *t);

// Verdadeiro se existe um CSV pronto para baixar
bool logger_has_data(void);

// Envia o CSV
// Avaliando a necessidade
esp_err_t logger_send_csv(httpd_req_t *req);
