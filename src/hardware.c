#include "display.h"
#include "network.h"
#include "config.h"
#include "esp_adc/adc_oneshot.h"

static adc_oneshot_unit_handle_t adc1_handle = NULL;

void hardware_init(void){
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL<<RL1_GPIO) | (1ULL<<RL2_GPIO) | (1ULL<<RL3_GPIO) | (1ULL<<RL4_GPIO) | (1ULL<<RL5_GPIO) | (1ULL<<LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_conf);
    gpio_set_level(RL1_GPIO, 0);
    gpio_set_level(RL2_GPIO, 0);
    gpio_set_level(RL3_GPIO, 0);
    gpio_set_level(RL4_GPIO, 0);
    gpio_set_level(RL5_GPIO, 0);
    gpio_set_level(LED_GPIO, 0);

    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL<<EMERGENCY_BTN_GPIO) | (1ULL<<CHARGE_BTN_GPIO) | (1ULL<<DISCHARGE_BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&in_conf);

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_ID,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTENUATION,
    };
    adc_oneshot_config_channel(adc1_handle, VOLTAGE_GPIO, &adc_config);
    adc_oneshot_config_channel(adc1_handle, CURRENT_GPIO, &adc_config);

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
    gpio_set_level(LED_GPIO, 1);
}