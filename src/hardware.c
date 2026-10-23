#include "hardware.h"
#include "display.h"
#include "config.h"

adc_oneshot_unit_handle_t adc1_handle = NULL;
batteryData batteries[2] = {
    [0] = {"Nancy 42V", 42.0f, 8.0f},
    [1] = {"Aveiro 12V", 12.0f, 3.0f}
};

void hardwareInit(void){
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL<<DISCHARGING_GATE_GPIO) | (1ULL<<CHARGING_42_GATE_GPIO) | (1ULL<<CHARGING_12_GATE_GPIO) | 
                        (1ULL<<DG1_GPIO) | (1ULL<<DG2_GPIO) | (1ULL<<DG3_GPIO) | (1ULL<<FAN_GPIO) | 
                        (1ULL<<LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_conf);

    gpio_set_level(CHARGING_42_GATE_GPIO, 0);
    gpio_set_level(CHARGING_12_GATE_GPIO, 0);
    gpio_set_level(DISCHARGING_GATE_GPIO, 0);
    gpio_set_level(DG1_GPIO, 0);
    gpio_set_level(DG2_GPIO, 0);
    gpio_set_level(DG3_GPIO, 0);
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
    const float readValue = 5*(raw/4095.0);
    return readValue;
}

float calculateVoltage(void){
    int raw = 0;
    adc_oneshot_read(adc1_handle, VOLTAGE_GPIO, &raw);
    const float readValue = 42*(raw/4095.0);
    return readValue;
}

float calculateTemperature(void){
    return 0;
}

int menuSelectBattery(void){
    int raw = 0;
    adc_oneshot_read(adc1_handle, POTENCIOMETER_GPIO, &raw);
    int readValue = (int) (2-1)*(raw/4095.0);
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