#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "control.h"
#include "main.h"

#define WIFI_SSID      "ESP32_BMS_ECO"
#define WIFI_PASS      "12345678"
#define LOG_FILE_PATH  "/storage/logs.bin"

static const char index_html[] =
    "<!DOCTYPE html>\n"
    "<html lang=\"pt-BR\">\n"
    "<head>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "    <title>BMS ESP32 - Dashboard & Controle</title>\n"
    "    <script src=\"https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4\"></script>\n"
    "    <script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n"
    "</head>\n"
    "<body class=\"bg-slate-900 text-slate-100 min-h-screen p-4 md:p-8 font-sans\">\n"
    "    <div class=\"max-w-6xl mx-auto space-y-6\">\n"
    "        \n"
    "        <header class=\"flex flex-col sm:flex-row justify-between items-center bg-slate-800 p-6 rounded-2xl border border-slate-700 shadow-xl gap-4\">\n"
    "            <div>\n"
    "                <h1 class=\"text-2xl font-bold text-sky-400\">Painel de Controle BMS</h1>\n"
    "                <p class=\"text-xs text-slate-400 mt-1\">Monitoramento e Telemetria em Tempo Real</p>\n"
    "            </div>\n"
    "            <button onclick=\"triggerEmergency()\" class=\"w-full sm:w-auto bg-red-600 hover:bg-red-700 active:scale-95 text-white font-bold py-3 px-6 rounded-xl shadow-lg transition-all border border-red-500 flex items-center justify-center gap-2 cursor-pointer\">\n"
    "                <svg class=\"w-5 h-5\" fill=\"none\" stroke=\"currentColor\" viewBox=\"0 0 24 24\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z\"></path></svg>\n"
    "                VÁLVULA DE EMERGÊNCIA\n"
    "            </button>\n"
    "        </header>\n"
    "\n"
    "        <div class=\"grid grid-cols-1 lg:grid-cols-3 gap-6\">\n"
    "            \n"
    "            <!-- Painel de Configuração / Bloqueado durante operação -->\n"
    "            <div id=\"controlPanel\" class=\"bg-slate-800 p-6 rounded-2xl border border-slate-700 shadow-xl space-y-6 transition-all\">\n"
    "                <div class=\"flex justify-between items-center border-b border-slate-700 pb-3\">\n"
    "                    <h2 class=\"text-lg font-semibold text-slate-200\">Operação da Bateria</h2>\n"
    "                    <span id=\"opBadge\" class=\"hidden text-xs font-mono px-2.5 py-1 rounded-full bg-amber-500/10 text-amber-400 border border-amber-500/20 animate-pulse\">Em Execução</span>\n"
    "                </div>\n"
    "                \n"
    "                <div class=\"space-y-4\">\n"
    "                    <div>\n"
    "                        <label class=\"block text-sm font-medium text-slate-400 mb-2\">Bateria Selecionada</label>\n"
    "                        <select id=\"batterySelect\" onchange=\"updateBatteryLimits()\" class=\"w-full bg-slate-900 border border-slate-700 text-slate-200 rounded-xl p-3 focus:outline-none focus:border-sky-500 transition-colors\">\n"
    "                            <option value=\"0\">Nancy 42V (Bateria 1)</option>\n"
    "                            <option value=\"1\">Aveiro 12V (Bateria 2)</option>\n"
    "                        </select>\n"
    "                    </div>\n"
    "\n"
    "                    <div>\n"
    "                        <label class=\"block text-sm font-medium text-slate-400 mb-2\">Voltagem Final (V)</label>\n"
    "                        <input type=\"number\" id=\"targetVoltage\" step=\"0.1\" value=\"42.0\" class=\"w-full bg-slate-900 border border-slate-700 text-slate-200 rounded-xl p-3 focus:outline-none focus:border-sky-500 transition-colors\">\n"
    "                        <p id=\"voltageHint\" class=\"text-xs text-slate-500 mt-1\">Intervalo seguro: 8.0V - 42.0V</p>\n"
    "                    </div>\n"
    "\n"
    "                    <div id=\"actionButtons\" class=\"grid grid-cols-2 gap-3 pt-2\">\n"
    "                        <button onclick=\"startProcess('charge')\" class=\"bg-emerald-600 hover:bg-emerald-700 active:scale-95 font-semibold py-3 px-4 rounded-xl transition-all text-center shadow-lg border border-emerald-500 cursor-pointer\">\n"
    "                            Carregar\n"
    "                        </button>\n"
    "                        <button onclick=\"startProcess('discharge')\" class=\"bg-amber-600 hover:bg-amber-700 active:scale-95 font-semibold py-3 px-4 rounded-xl transition-all text-center shadow-lg border border-amber-500 cursor-pointer\">\n"
    "                            Descarregar\n"
    "                        </button>\n"
    "                    </div>\n"
    "                </div>\n"
    "\n"
    "                <div class=\"border-t border-slate-700 pt-6\">\n"
    "                    <h3 class=\"text-sm font-medium text-slate-400 mb-3\">Exportar Histórico</h3>\n"
    "                    <button onclick=\"downloadBinAsCSV()\" class=\"w-full bg-slate-700 hover:bg-slate-600 active:scale-95 text-slate-200 font-semibold py-3 px-4 rounded-xl transition-all flex items-center justify-center gap-2 border border-slate-600 shadow-md cursor-pointer\">\n"
    "                        <svg class=\"w-5 h-5 text-sky-400\" fill=\"none\" stroke=\"currentColor\" viewBox=\"0 0 24 24\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4\"></path></svg>\n"
    "                        Baixar Log (.CSV)\n"
    "                    </button>\n"
    "                </div>\n"
    "            </div>\n"
    "\n"
    "            <!-- Painel do Gráfico e Operação Atual / Barra de Progresso -->\n"
    "            <div class=\"lg:col-span-2 bg-slate-800 p-6 rounded-2xl border border-slate-700 shadow-xl flex flex-col space-y-4\">\n"
    "                <div class=\"flex justify-between items-center border-b border-slate-700 pb-3\">\n"
    "                    <div>\n"
    "                        <h2 id=\"operationTitle\" class=\"text-lg font-semibold text-slate-200\">Status da Operação</h2>\n"
    "                        <p id=\"operationStatusText\" class=\"text-xs text-slate-400\">Aguardando início de comando...</p>\n"
    "                    </div>\n"
    "                    <span id=\"statusBadge\" class=\"text-xs font-mono px-2.5 py-1 rounded-full bg-emerald-500/10 text-emerald-400 border border-emerald-500/20\">Conectado</span>\n"
    "                </div>\n"
    "\n"
    "                <!-- Seção da Barra de Progresso baseada na fórmula do firmware -->\n"
    "                <div id=\"progressSection\" class=\"bg-slate-900/50 p-4 rounded-xl border border-slate-700/50 space-y-2\">\n"
    "                    <div class=\"flex justify-between text-xs font-medium\">\n"
    "                        <span id=\"progressLabel\" class=\"text-slate-400\">Progresso</span>\n"
    "                        <span id=\"progressPercent\" class=\"text-sky-400 font-mono\">0.0%</span>\n"
    "                    </div>\n"
    "                    <div class=\"w-full bg-slate-900 rounded-full h-3 overflow-hidden border border-slate-700\">\n"
    "                        <div id=\"progressBar\" class=\"bg-sky-500 h-full transition-all duration-300 w-0\"></div>\n"
    "                    </div>\n"
    "                </div>\n"
    "\n"
    "                <div class=\"relative flex-1 min-h-[260px]\">\n"
    "                    <canvas id=\"telemetryChart\"></canvas>\n"
    "                </div>\n"
    "            </div>\n"
    "        </div>\n"
    "    </div>\n"
    "\n"
    "    <script>\n"
    "        let activeMode = null;        // 'charge' ou 'discharge'\n"
    "        let initialVoltage = 0.0;     // Tensão no início da operação\n"
    "        let finalVoltage = 0.0;       // Tensão final desejada (definida pelo usuário)\n"
    "\n"
    "        const batteryLimits = {\n"
    "            '0': { min: 8.0, max: 42.0, default: 42.0 },\n"
    "            '1': { min: 3.0, max: 12.0, default: 12.0 }\n"
    "        };\n"
    "\n"
    "        function updateBatteryLimits() {\n"
    "            const val = document.getElementById('batterySelect').value;\n"
    "            const limits = batteryLimits[val];\n"
    "            const targetInput = document.getElementById('targetVoltage');\n"
    "            targetInput.min = limits.min;\n"
    "            targetInput.max = limits.max;\n"
    "            targetInput.value = limits.default;\n"
    "            document.getElementById('voltageHint').innerText = `Intervalo seguro: ${limits.min}V - ${limits.max}V`;\n"
    "        }\n"
    "\n"
    "        // Chart.js Setup\n"
    "        const ctx = document.getElementById('telemetryChart').getContext('2d');\n"
    "        const chart = new Chart(ctx, {\n"
    "            type: 'line',\n"
    "            data: {\n"
    "                labels: [],\n"
    "                datasets: [\n"
    "                    { label: 'Voltagem (V)', borderColor: '#38bdf8', backgroundColor: 'rgba(56, 189, 248, 0.1)', data: [], borderWidth: 2, fill: true, tension: 0.2 },\n"
    "                    { label: 'Corrente (A)', borderColor: '#f59e0b', backgroundColor: 'rgba(245, 158, 11, 0.1)', data: [], borderWidth: 2, fill: true, tension: 0.2 }\n"
    "                ]\n"
    "            },\n"
    "            options: {\n"
    "                responsive: true,\n"
    "                maintainAspectRatio: false,\n"
    "                scales: {\n"
    "                    x: { grid: { color: '#334155' }, ticks: { color: '#94a3b8' } },\n"
    "                    y: { grid: { color: '#334155' }, ticks: { color: '#94a3b8' } }\n"
    "                },\n"
    "                plugins: { legend: { labels: { color: '#cbd5e1' } } }\n"
    "            }\n"
    "        });\n"
    "\n"
    "        // Polling binary data every second & calculating progress\n"
    "        async function updateTelemetry() {\n"
    "            try {\n"
    "                const res = await fetch('/api/telemetry');\n"
    "                if (!res.ok) return;\n"
    "                const buffer = await res.arrayBuffer();\n"
    "                const view = new DataView(buffer);\n"
    "                const totalRecords = buffer.byteLength / 12;\n"
    "                if (totalRecords === 0) return;\n"
    "\n"
    "                // Tensão atual vem do último registro do .bin\n"
    "                const lastOffset = (totalRecords - 1) * 12;\n"
    "                const nowVoltage = view.getFloat32(lastOffset + 8, true);\n"
    "\n"
    "                // Se houver uma operação ativa, calcula a porcentagem exata usando a fórmula\n"
    "                if (activeMode && initialVoltage !== finalVoltage) {\n"
    "                    let percentage = 0;\n"
    "                    if (activeMode === 'charge') {\n"
    "                        percentage = ((nowVoltage - initialVoltage) / (finalVoltage - initialVoltage)) * 100;\n"
    "                    } else if (activeMode === 'discharge') {\n"
    "                        percentage = ((initialVoltage - nowVoltage) / (initialVoltage - finalVoltage)) * 100;\n"
    "                    }\n"
    "                    // Limita entre 0% e 100%\n"
    "                    percentage = Math.max(0, Math.min(100, percentage));\n"
    "\n"
    "                    document.getElementById('progressBar').style.width = percentage.toFixed(1) + '%';\n"
    "                    document.getElementById('progressPercent').innerText = percentage.toFixed(1) + '%';\n"
    "                    document.getElementById('operationStatusText').innerText = `Tensão Atual: ${nowVoltage.toFixed(2)}V / Alvo: ${finalVoltage}V`;\n"
    "                }\n"
    "                \n"
    "                const maxPoints = 30;\n"
    "                const start = Math.max(0, totalRecords - maxPoints);\n"
    "                \n"
    "                const times = [], voltages = [], currents = [];\n"
    "                for (let i = start; i < totalRecords; i++) {\n"
    "                    const offset = i * 12;\n"
    "                    times.push(view.getFloat32(offset, true).toFixed(0) + 's');\n"
    "                    currents.push(view.getFloat32(offset + 4, true).toFixed(2));\n"
    "                    voltages.push(view.getFloat32(offset + 8, true).toFixed(2));\n"
    "                }\n"
    "\n"
    "                chart.data.labels = times;\n"
    "                chart.data.datasets[0].data = voltages;\n"
    "                chart.data.datasets[1].data = currents;\n"
    "                chart.update('none');\n"
    "            } catch (e) {}\n"
    "        }\n"
    "        setInterval(updateTelemetry, 1000);\n"
    "\n"
    "        // Emergency Control\n"
    "        async function triggerEmergency() {\n"
    "            await fetch('/api/emergency', { method: 'POST' });\n"
    "            alert('Válvula de emergência acionada!');\n"
    "            activeMode = null;\n"
    "            document.getElementById('batterySelect').disabled = false;\n"
    "            document.getElementById('targetVoltage').disabled = false;\n"
    "            document.getElementById('actionButtons').style.opacity = '1';\n"
    "            document.getElementById('actionButtons').style.pointerEvents = 'auto';\n"
    "            document.getElementById('opBadge').classList.add('hidden');\n"
    "            document.getElementById('operationTitle').innerText = 'Status da Operação';\n"
    "            document.getElementById('operationStatusText').innerText = 'Operação interrompida por emergência.';\n"
    "            document.getElementById('progressBar').style.width = '0%';\n"
    "            document.getElementById('progressPercent').innerText = '0.0%';\n"
    "        }\n"
    "\n"
    "        // Start Charging / Discharging\n"
    "        async function startProcess(mode) {\n"
    "            const bat = document.getElementById('batterySelect').value;\n"
    "            const volt = parseFloat(document.getElementById('targetVoltage').value);\n"
    "            \n"
    "            finalVoltage = volt;\n"
    "            activeMode = mode;\n"
    "\n"
    "            // Pega a última voltagem registrada ou assume valor padrão inicial para o cálculo da fórmula\n"
    "            try {\n"
    "                const res = await fetch('/api/telemetry');\n"
    "                const buffer = await res.arrayBuffer();\n"
    "                const view = new DataView(buffer);\n"
    "                const totalRecords = buffer.byteLength / 12;\n"
    "                if (totalRecords > 0) {\n"
    "                    initialVoltage = view.getFloat32((totalRecords - 1) * 12 + 8, true);\n"
    "                } else {\n"
    "                    initialVoltage = (mode === 'charge') ? (bat == '0' ? 30.0 : 10.0) : volt;\n"
    "                }\n"
    "            } catch(e) {\n"
    "                initialVoltage = volt > 20 ? 35.0 : 11.0;\n"
    "            }\n"
    "\n"
    "            // Trava interface\n"
    "            document.getElementById('batterySelect').disabled = true;\n"
    "            document.getElementById('targetVoltage').disabled = true;\n"
    "            document.getElementById('actionButtons').style.opacity = '0.5';\n"
    "            document.getElementById('actionButtons').style.pointerEvents = 'none';\n"
    "            document.getElementById('opBadge').classList.remove('hidden');\n"
    "            document.getElementById('operationTitle').innerText = mode === 'charge' ? 'CARREGANDO' : 'DESCARREGANDO';\n"
    "\n"
    "            await fetch(`/api/control?action=${mode}&battery=${bat}&voltage=${volt}`, { method: 'POST' });\n"
    "        }\n"
    "\n"
    "        // Download CSV\n"
    "        async function downloadBinAsCSV() {\n"
    "            try {\n"
    "                const res = await fetch('/api/telemetry');\n"
    "                const buffer = await res.arrayBuffer();\n"
    "                const view = new DataView(buffer);\n"
    "                const totalRecords = buffer.byteLength / 12;\n"
    "\n"
    "                let csv = 'Timestamp,Current,Voltage\\n';\n"
    "                for (let i = 0; i < totalRecords; i++) {\n"
    "                    const offset = i * 12;\n"
    "                    const t = view.getFloat32(offset, true);\n"
    "                    const c = view.getFloat32(offset + 4, true);\n"
    "                    const v = view.getFloat32(offset + 8, true);\n"
    "                    csv += `${t},${c},${v}\\n`;\n"
    "                }\n"
    "\n"
    "                const blob = new Blob([csv], { type: 'text/csv' });\n"
    "                const url = window.URL.createObjectURL(blob);\n"
    "                const a = document.createElement('a');\n"
    "                a.href = url;\n"
    "                a.download = 'telemetria_bms.csv';\n"
    "                a.click();\n"
    "                window.URL.revokeObjectURL(url);\n"
    "            } catch(e) {}\n"
    "        }\n"
    "    </script>\n"
    "</body>\n"
    "</html>\n";

static esp_err_t root_get_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t telemetry_bin_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "application/octet-stream");
    FILE *f = fopen(LOG_FILE_PATH, "rb");
    if (f == NULL) {
        return httpd_resp_send_404(req);
    }
    char buffer[512];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        httpd_resp_send_chunk(req, buffer, bytes_read);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t emergency_post_handler(httpd_req_t *req){
    emergencyValve();
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t control_post_handler(httpd_req_t *req) {
    char buf[128];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char action[16] = {0}, bat_str[8] = {0}, volt_str[16] = {0};
        httpd_query_key_value(buf, "action", action, sizeof(action));
        httpd_query_key_value(buf, "battery", bat_str, sizeof(bat_str));
        httpd_query_key_value(buf, "voltage", volt_str, sizeof(volt_str));

        int battery = atoi(bat_str);
        float voltage = atof(volt_str);
        operationFinalVoltage = voltage;

        if (strcmp(action, "charge") == 0){
            int TYPE = (battery == 0) ? 42 : 12;
            startCharge(&CHARGING_STATE, TYPE);
        } else if (strcmp(action, "discharge") == 0) {
            startDischarge(&DISCHARGING_STATE);
        }
    }
    return httpd_resp_sendstr(req, "OK");
}

void start_webserver(void){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };
        httpd_register_uri_handler(server, &root_uri);

        httpd_uri_t telemetry_uri = { .uri = "/api/telemetry", .method = HTTP_GET, .handler = telemetry_bin_handler };
        httpd_register_uri_handler(server, &telemetry_uri);

        httpd_uri_t emergency_uri = { .uri = "/api/emergency", .method = HTTP_POST, .handler = emergency_post_handler };
        httpd_register_uri_handler(server, &emergency_uri);

        httpd_uri_t control_uri = { .uri = "/api/control", .method = HTTP_POST, .handler = control_post_handler };
        httpd_register_uri_handler(server, &control_uri);
    }
}

void wifi_init_softap(void){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}