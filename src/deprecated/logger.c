// =============================================================================
// logger.c  -  "Como guardo os dados e gero o CSV?"
//
// - logger_start()      abre o arquivo e escreve o cabeçalho
// - logger_add_sample() acrescenta uma linha (chamada a cada 1 s)
// - logger_stop()       fecha o arquivo
// - logger_send_csv()   envia o arquivo ao celular em pedaços
//
// Este módulo só LÊ a telemetria. Nunca aciona relé.
// =============================================================================
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_littlefs.h"

#include "config.h"
#include "logger.h"
#include "control.h"

static const char *TAG = "logger";

#define FLUSH_EVERY_N_SAMPLES   10      
#define SEND_CHUNK_BYTES        512    
#define MUTEX_TIMEOUT_MS        200

static SemaphoreHandle_t s_mutex     = NULL;   
static FILE             *s_file      = NULL;   
static bool              s_has_data  = false; 
static volatile bool     s_sending   = false;  
static int               s_unflushed = 0;   


// Funções internas


// Fecha o arquivo se estiver aberto 
static void close_file_locked(void)
{
    if (s_file != NULL) {
        fclose(s_file);
        s_file = NULL;
        s_unflushed = 0;
    }
}


// API pública

esp_err_t logger_init(void)
{
    // Cria o mutex
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }
    // Configura o CSV
    esp_vfs_littlefs_conf_t conf = {
        .base_path              = LOG_MOUNT_POINT,
        .partition_label        = LOG_PARTITION_LABEL,
        .format_if_mount_failed = true,     // primeira vez: formata a partição
        .dont_mount             = false,
    };

    // Verifica erros
    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao montar LittleFS: %s", esp_err_to_name(err));
        return err;
    }

    // Organiza os dados
    size_t total = 0, used = 0;
    if (esp_littlefs_info(LOG_PARTITION_LABEL, &total, &used) == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS montado: %u de %u bytes usados",
                 (unsigned)used, (unsigned)total);
    }

    // Se sobrou um CSV de antes de reiniciar, ele continua disponível
    struct stat st;
    if (stat(LOG_FILE_PATH, &st) == 0 && st.st_size > (off_t)strlen(LOG_CSV_HEADER)) {
        s_has_data = true;
    }
    return ESP_OK;
}

esp_err_t logger_start(void)
{
    // espera o download terminar
    if (s_sending) {
        return ESP_ERR_INVALID_STATE;       
    }
    // Verifica se o mutex está fechado
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    close_file_locked();
    s_has_data = false;

    // Confere o espaço livre antes de começar
    size_t total = 0, used = 0;
    esp_err_t err = esp_littlefs_info(LOG_PARTITION_LABEL, &total, &used);
    if (err != ESP_OK) {
        xSemaphoreGive(s_mutex);
        return err;
    }
    // O arquivo antigo será apagado, então o espaço dele conta como livre
    struct stat st;
    size_t old_size = (stat(LOG_FILE_PATH, &st) == 0) ? (size_t)st.st_size : 0;
    size_t free_bytes = total - used + old_size;
    if (free_bytes < LOG_MIN_FREE_BYTES) {
        ESP_LOGE(TAG, "Pouco espaço livre: %u bytes", (unsigned)free_bytes);
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NO_MEM;
    }

    // Apaga a sessão anterior
    s_file = fopen(LOG_FILE_PATH, "w");     
    if (s_file == NULL) {
        ESP_LOGE(TAG, "Não foi possível criar %s", LOG_FILE_PATH);
        xSemaphoreGive(s_mutex);
        return ESP_FAIL;
    }

    // Verifica acesso ao arquivo
    if (fputs(LOG_CSV_HEADER, s_file) < 0) {
        close_file_locked();
        xSemaphoreGive(s_mutex);
        return ESP_FAIL;
    }
    fflush(s_file);

    s_has_data = true;
    ESP_LOGI(TAG, "Sessão iniciada");
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

void logger_stop(void)
{
    // Verifica se o mutex está fechado
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return;
    }
    // Verifica o arquivo
    if (s_file != NULL) {
        fflush(s_file);
        close_file_locked();
        ESP_LOGI(TAG, "Sessão encerrada");
    }
    // Abre o mutex
    xSemaphoreGive(s_mutex);
}

void logger_add_sample(const telemetry_t *t)
{
    if (t == NULL) {
        return;
    }
    // Verifica se o mutex está fechado
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return;
    }
    // Verifica a disponibilidade do arquivo
    if (s_file == NULL) {
        xSemaphoreGive(s_mutex);
        return;
    }

    // tempo_s,estado,tensao_V,corrente_A,temp_C,capacidade_Ah,energia_Wh
    int n = fprintf(s_file, "%lu,%s,%.3f,%.3f,%.1f,%.4f,%.3f\n",
                    (unsigned long)t->elapsed_s,
                    state_to_text(t->state),
                    t->voltage, t->current, t->temp, t->ah, t->wh);

    if (n < 0) {
        // Disco cheio ou erro de escrita
        ESP_LOGE(TAG, "Erro ao escrever no arquivo");
        close_file_locked();
    } 
    // Salva dados em caso de falta de alimentação
    else if (++s_unflushed >= FLUSH_EVERY_N_SAMPLES) {
        fflush(s_file);
        s_unflushed = 0;
    }
    xSemaphoreGive(s_mutex);
}

bool logger_has_data(void)
{
    return s_has_data;
}

esp_err_t logger_send_csv(httpd_req_t *req)
{
    if (!s_has_data) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Sem dados");
        return ESP_FAIL;
    }

    // Se a sessão ainda estiver aberta, grava o que está no buffer e marca o download
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Ocupado");
        return ESP_FAIL;
    }
    if (s_file != NULL) {
        fflush(s_file);
        s_unflushed = 0;
    }
    s_sending = true;                      
    xSemaphoreGive(s_mutex);

    FILE *f = fopen(LOG_FILE_PATH, "r");
    if (f == NULL) {
        s_sending = false;
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Arquivo não encontrado");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition",
                       "attachment; filename=\"bateria.csv\"");

    char buf[SEND_CHUNK_BYTES];
    esp_err_t err = ESP_OK;
    size_t n;

    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        err = httpd_resp_send_chunk(req, buf, n);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Download interrompido: %s", esp_err_to_name(err));
            break;
        }
    }
    fclose(f);

    if (err == ESP_OK) {
        err = httpd_resp_send_chunk(req, NULL, 0);
    }
    s_sending = false;
    return err;
}