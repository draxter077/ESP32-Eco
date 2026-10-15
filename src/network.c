#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include "network.h"

#define EXAMPLE_ESP_WIFI_SSID      "EcoOctano_Telemetria"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       4

extern void acionar_emergencia(void);
extern CSV_FILE_PATH;
typedef struct {
    int timestamp;
    float voltage;
    float current;
} TelemetryBinarySample;

// Inicializa o wifi
void wifi_init_softap(void){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    if(strlen(EXAMPLE_ESP_WIFI_PASS) == 0) wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}
//



// Gestão do  WebServer
const char* html_template = 
    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<title>Eco Octano - Telemetria</title>"
    "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"
    "<style>"
    "body { font-family: Arial; text-align: center; background: #f4f4f9; margin: 0; padding: 20px; }"
    ".card { background: white; padding: 20px; margin: 10px auto; width: 90%; max-width: 600px; border-radius: 8px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1); }"
    "button { padding: 10px 20px; margin: 5px; font-size: 16px; cursor: pointer; border: none; border-radius: 5px; background: #007bff; color: white; }"
    "button.stop { background: #dc3545; }"
    "</style></head><body>"
    "<h1>Eco Octano - Controle de Carga/Descarga</h1>"
    "<div class='card'>"
    "  <h3>Status Atual</h3>"
    "  <p>Voltagem: <span id='v'>--</span> V | Corrente: <span id='i'>--</span> A</p>"
    "</div>"
    "<div class='card'>"
    "  <h3>Ações</h3>"
    "  <button class='stop' onclick=\"fetch('/cmd?action=stop')\">PARADA DE EMERGÊNCIA</button>"
    "</div>"
    "<div class='card'>"
    "  <h3>Gráfico de Descarga</h3>"
    "  <canvas id='telemetryChart'></canvas>"
    "  <br><a href='/download-csv'><button>Baixar CSV</button></a>"
    "</div>"
    "<script>"
    "const ctx = document.getElementById('telemetryChart').getContext('2d');"
    "const chart = new Chart(ctx, { type: 'line', data: { labels: [], datasets: [{ label: 'Voltagem (V)', data: [], borderColor: 'rgb(75, 192, 192)' }] } });"
    "setInterval(async () => {"
    "  let res = await fetch('/data');"
    "  let json = await res.json();"
    "  document.getElementById('v').innerText = json.v;"
    "  document.getElementById('i').innerText = json.i;"
    "  let now = new Date().toLocaleTimeString();"
    "  if (chart.data.labels.length > 20) { chart.data.labels.shift(); chart.data.datasets[0].data.shift(); }"
    "  chart.data.labels.push(now);"
    "  chart.data.datasets[0].data.push(json.v);"
    "  chart.update();"
    "}, 1000);"
    "</script>"
    "</body></html>";

static esp_err_t index_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_template, strlen(html_template));
    return ESP_OK;
}

static esp_err_t data_get_handler(httpd_req_t *req) {
    float latest_v = 0.0;
    float latest_i = 0.0;

    FILE *f = fopen(CSV_FILE_PATH, "rb");
    if (f != NULL){
        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);

        if (file_size >= sizeof(TelemetryBinarySample)) {
            fseek(f, -((long)sizeof(TelemetryBinarySample)), SEEK_END);
            
            TelemetryBinarySample last_sample;
            if (fread(&last_sample, sizeof(TelemetryBinarySample), 1, f) == 1) {
                latest_v = last_sample.voltage;
                latest_i = last_sample.current;
            }
        }
        fclose(f);
    }

    char json_buffer[100];
    snprintf(json_buffer, sizeof(json_buffer), "{\"v\":%.2f, \"i\":%.2f}", latest_v, latest_i);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_buffer, strlen(json_buffer));
    return ESP_OK;
}

static esp_err_t csv_get_handler(httpd_req_t *req) {
    FILE *f = fopen(CSV_FILE_PATH, "rb");

    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=telemetria_convertida.csv");

    httpd_resp_send_chunk(req, "Tempo(s),Tensao(V),Corrente(A)\n", HTTPD_RESP_USE_STRLEN);

    TelemetryBinarySample sample;
    char line_buffer[64];

    while (fread(&sample, sizeof(TelemetryBinarySample), 1, f) == 1){
        int len = snprintf(line_buffer, sizeof(line_buffer), "%d,%.2f,%.2f\n", 
                           sample.timestamp, 
                           sample.voltage, 
                           sample.current);
        
        if (len > 0 && httpd_resp_send_chunk(req, line_buffer, len) != ESP_OK){
            fclose(f);
            return ESP_FAIL;
        }
    }

    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t cmd_get_handler(httpd_req_t *req) {
    char buf[100];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        if (strstr(buf, "action=stop")) {
            acionar_emergencia();
        }
    }
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t index_uri = { .uri = "/",             .method = HTTP_GET, .handler = index_get_handler };
        httpd_uri_t data_uri  = { .uri = "/data",         .method = HTTP_GET, .handler = data_get_handler };
        httpd_uri_t csv_uri   = { .uri = "/download-csv", .method = HTTP_GET, .handler = csv_get_handler };
        httpd_uri_t cmd_uri   = { .uri = "/cmd",          .method = HTTP_GET, .handler = cmd_get_handler };
        
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &data_uri);
        httpd_register_uri_handler(server, &csv_uri);
        httpd_register_uri_handler(server, &cmd_uri);
    }
    return server;
}
//