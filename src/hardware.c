#include "hardware.h"

float calculateCurrent(void){
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, CURRENT_GPIO, &raw));    
    return ((raw/4095.0)*3.3/10.0)/0.1;
}

float calculateVoltage(int batterySelected){
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, VOLTAGE_GPIO, &raw));
    return ((raw/4095.0)*3.3)*(39.0+3.3)/3.3; // adicionar especificidade da bateria
}

float calculateTemperature(void){
    return 0;
}

int menuSelectBattery(void){
    return 0;
}

int menuSelectFinalVoltage(void){
    return 0;
}

void hardwareInit(void){
}