/*
Como o celular fala com o ESP32?
*/

#pragma once

#include "config.h"
#include "esp_err.h"
#include <stddef.h>

// Inicia Wi-Fi , mDNS e o servidor HTTP
esp_err_t network_init(void);

// Envia a telemetria em **JSON** a todos os celulares conectados no WebSocket.
// Chamada por task_interface a cada 500 ms.
void network_send_telemetry(const telemetry_t *t);
