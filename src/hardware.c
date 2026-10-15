#include "display.h"
#include "network.h"
#include "config.h"
#include "esp_adc/adc_oneshot.h"

static adc_oneshot_unit_handle_t adc1_handle = NULL;

void hardware_init(void){
  network_init();
  display_init();
}

void read_voltage(void){
        int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, VOLTAGE_GPIO, &raw));
    return ((raw/4095.0)*3.3)*(39.0+3.3)/3.3;
}

void read_current(void){
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, CURRENT_GPIO, &raw));    
    return ((raw/4095.0)*3.3/10.0)/0.1;
}

void read_temperature(void){}

void read_potenciometer(void){}

void calibrate_current_zero(void){}

void emergency_valve(void){
    gpio_set_level(RL1_GPIO, 0);
    gpio_set_level(RL2_GPIO, 0);
    gpio_set_level(RL3_GPIO, 0);
    gpio_set_level(RL4_GPIO, 0);
    gpio_set_level(RL5_GPIO, 0);
    gpio_set_level(FAN_GPIO, 0);
    gpio_set_level(LED_ERROR_GPIO, 0);
}