idf_component_register(
    SRCS "main.c" "hardware.c" "control.c" "menu.c" "display.c" "telemetry.c" "network.c"
    INCLUDE_DIRS "."
    REQUIRES 
        esp_http_server 
        nvs_flash 
        driver 
        esp_adc 
        esp_driver_gpio
        esp_wifi
        esp_netif
        spiffs
)