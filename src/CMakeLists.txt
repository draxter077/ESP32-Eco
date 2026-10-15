# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS "main.c" "hardware.c" "control.c" "logger.c" "network.c" "display.c"
    INCLUDE_DIRS "."
    REQUIRES 
        esp_http_server 
        nvs_flash 
        driver 
        esp_adc 
        esp_driver_gpio
        SSD1306
)
