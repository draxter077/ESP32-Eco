#include "hardware.h"
#include "display.h"
#include "config.h"

adc_oneshot_unit_handle_t adc1_handle = NULL;

void hardwareInit(void){
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL<<GATE1_GPIO) | (1ULL<<GATE2_GPIO) | (1ULL<<GATE3_GPIO) | 
                        (1ULL<<GATE4_GPIO) | (1ULL<<GATE5_GPIO) | (1ULL<<FAN_GPIO) | 
                        (1ULL<<LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_conf);

    gpio_set_level(GATE1_GPIO, 0);
    gpio_set_level(GATE2_GPIO, 0);
    gpio_set_level(GATE3_GPIO, 0);
    gpio_set_level(GATE4_GPIO, 0);
    gpio_set_level(GATE5_GPIO, 0);
    gpio_set_level(FAN_GPIO, 0);
    gpio_set_level(LED_GPIO, 0);

    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL<<EMERGENCY_BTN_GPIO) | (1ULL<<ENTER_BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
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
    adc_oneshot_config_channel(adc1_handle, POTENCIOMETER_GPIO, &adc_config);

    displayInit();
}

float calculateCurrent(void){
    int raw = 0;
    adc_oneshot_read(adc1_handle, CURRENT_GPIO, &raw);
    return ((raw/4095.0)*3.3/10.0)/0.1;
}

float calculateVoltage(int batterySelected){
    int raw = 0;
    adc_oneshot_read(adc1_handle, VOLTAGE_GPIO, &raw);
    return ((raw/4095.0)*3.3)*(39.0+3.3)/3.3; // adicionar especificidade da bateria
}

float calculateTemperature(void){
    return 0;
}

int menuSelectBattery(void){
    int raw = 0;
    adc_oneshot_read(adc1_handle, POTENCIOMETER_GPIO, &raw);
    int readValue = (int) (5-1)*(raw/4095.0);
    displayBatterySelect(readValue);
    return readValue;
}

int menuSelectFinalVoltage(int batterySelected, float nowVoltage){
    int raw = 0;
    batteryData b = batteries[batterySelected];
    adc_oneshot_read(adc1_handle, POTENCIOMETER_GPIO, &raw);
    int readValue = (int) (b.maxVoltage - b.minVoltage)*(raw/4095.0) + b.minVoltage;
    displayFinalVoltageSelect(readValue, batterySelected, nowVoltage);
    return raw;
}