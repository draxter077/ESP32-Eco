#include "esp_http_server.h"

// Estrutura exata salva na Flash (exemplo com 3 campos: float, float, uint32_t)
typedef struct __attribute__((packed)) {
    float field1;          // ex: 0.0
    float field2;          // ex: 00.0
    uint32_t timestamp;    // ex: 000000.0
} log_record_t;

// Handler do Servidor HTTP para transmitir o binário
static esp_err_t download_binary_handler(httpd_req_t *req) {
    // Define o tipo de conteúdo como binário genérico
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"dados.bin\"");

    // Exemplo simulado: abrindo o arquivo binário da Flash (ou lendo da partição)
    FILE *f = fopen("/storage/logs.bin", "rb");
    if (f == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char chunk[512];
    size_t read_bytes;
    
    // Lê e envia em blocos para não estourar a RAM do ESP32
    while ((read_bytes = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
            fclose(f);
            return ESP_FAIL;
        }
    }

    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0); // Finaliza o stream
    return ESP_OK;
}




JS
async function baixarComoCSV() {
    try {
        // 1. Faz o fetch do arquivo binário direto do ESP32
        let response = await fetch('/download');
        let buffer = await response.arrayBuffer();

        // Cada registro tem o tamanho da estrutura em C (ex: 2 floats de 4 bytes + 1 uint32 de 4 bytes = 12 bytes)
        const recordSize = 12; 
        let view = new DataView(buffer);
        let csvContent = "Field1;Field2;Timestamp\n"; // Cabeçalho do CSV

        // 2. Percorre o buffer binário linha a linha no cliente
        for (let offset = 0; offset < buffer.byteLength; offset += recordSize) {
            // Ajuste o endianness se necessário (ESP32 é Little-Endian por padrão -> true)
            let field1 = view.getFloat32(offset, true);
            let field2 = view.getFloat32(offset + 4, true);
            let timestamp = view.getUint32(offset + 8, true);

            // Monta a linha no formato que você queria
            csvContent += `${field1.toFixed(1)};${field2.toFixed(1)};${timestamp}\n`;
        }

        // 3. Cria um Blob e força o download automático no navegador do cliente
        let blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
        let url = URL.createObjectURL(blob);
        let a = document.createElement('a');
        a.href = url;
        a.download = "dados_convertidos.csv";
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

    } catch (error) {
        console.error("Erro ao baixar ou converter os dados:", error);
    }
}