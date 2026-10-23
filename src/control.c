#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware.h"
#include "display.h"
#include "config.h"
#include "esp_timer.h"

void emergencyValve(){
    gpio_set_level(DISCHARGING_GATE_GPIO, 0);
    gpio_set_level(CHARGING_42_GATE_GPIO, 0);
    gpio_set_level(CHARGING_12_GATE_GPIO, 0);
    gpio_set_level(DG1_GPIO, 0);
    gpio_set_level(DG2_GPIO, 0);
    gpio_set_level(DG3_GPIO, 0);
    gpio_set_level(FAN_GPIO, 0);
    gpio_set_level(LED_GPIO, 1);
    displayInOperation(0,0,0,0,0,0,0);
    vTaskDelay(pdMS_TO_TICKS(5000));
}

void startCharge(int* CHARGING_STATE, int TYPE){
    *CHARGING_STATE = 1;
    if(TYPE == 42) gpio_set_level(CHARGING_42_GATE_GPIO, 1);
    else if(TYPE == 12) gpio_set_level(CHARGING_12_GATE_GPIO, 1);
}

void endCharge(int* CHARGING_STATE, int TYPE){
    *CHARGING_STATE = 0;
    if(TYPE == 42) gpio_set_level(CHARGING_42_GATE_GPIO, 0);
    else if(TYPE == 12) gpio_set_level(CHARGING_12_GATE_GPIO, 0);
}

void startDischarge(int* DISCHARGING_STATE){
    *DISCHARGING_STATE = 1;
    gpio_set_level(DISCHARGING_GATE_GPIO, 1);
    gpio_set_level(FAN_GPIO, 1);
}

void endDischarge(int* DISCHARGING_STATE){
    *DISCHARGING_STATE = 0;
    gpio_set_level(DISCHARGING_GATE_GPIO, 0);
    gpio_set_level(DG1_GPIO, 0);
    gpio_set_level(DG2_GPIO, 0);
    gpio_set_level(DG3_GPIO, 0);
    gpio_set_level(FAN_GPIO, 0);
}

void arrangeResistors(float nowVoltage, float operationFinalVoltage, float operationInitialVoltage, float nowCurrent, int batterySelected){
    if(nowVoltage > 30){
        gpio_set_level(DG1_GPIO, 0);
        gpio_set_level(DG2_GPIO, 1);
        gpio_set_level(DG3_GPIO, 1);
    }
    else if(nowVoltage > 14){
        gpio_set_level(DG1_GPIO, 0);
        gpio_set_level(DG2_GPIO, 1);
        gpio_set_level(DG3_GPIO, 0);
    }
    else if(nowVoltage > 10){
        gpio_set_level(DG1_GPIO, 0);
        gpio_set_level(DG2_GPIO, 0);
        gpio_set_level(DG3_GPIO, 0);
    }
    else if(nowVoltage > 6){
        gpio_set_level(DG1_GPIO, 1);
        gpio_set_level(DG2_GPIO, 1);
        gpio_set_level(DG3_GPIO, 0);
    }
    else{
        gpio_set_level(DG1_GPIO, 1);
        gpio_set_level(DG2_GPIO, 0);
        gpio_set_level(DG3_GPIO, 0);
    }
}

void operationCare(int* CHARGING_STATE, int* DISCHARGING_STATE, int* batterySelected, float* nowCurrent, float* nowVoltage, float* operationFinalVoltage, float* operationInitialVoltage, int64_t* lastTimeError){
    *nowCurrent = calculateCurrent();
    *nowVoltage = calculateVoltage();
    batteryData b = batteries[*batterySelected];

    int TYPE = 0;
    if(*batterySelected == 0) TYPE = 42;
    else if(*batterySelected == 1) TYPE = 12;

    // Salva telemetria

    displayInOperation(*CHARGING_STATE, *DISCHARGING_STATE, *operationInitialVoltage, *operationFinalVoltage, *nowVoltage, *nowCurrent, *batterySelected);
    if(gpio_get_level(EMERGENCY_BTN_GPIO) || ((*nowVoltage > b.maxVoltage*1.05 || *nowCurrent > 3) && esp_timer_get_time() - *lastTimeError > 5*1000*1000 && esp_timer_get_time() - *lastTimeError < 10*1000*1000)){
        *CHARGING_STATE = 0;
        *DISCHARGING_STATE = 0;
        emergencyValve();
        displayInitial();
    }
    else if(esp_timer_get_time() - *lastTimeError > 10*1000*1000 && *lastTimeError){
        *lastTimeError = 0;
    }
    else if((*nowVoltage > b.maxVoltage*1.05 || *nowCurrent > 3) && !(*lastTimeError)){
        *lastTimeError = esp_timer_get_time();
    }
    
    if(*CHARGING_STATE && *nowVoltage >= *operationFinalVoltage){
        endCharge(CHARGING_STATE, TYPE);
        displayInitial();
    }
    else if(*DISCHARGING_STATE && *nowVoltage <= *operationFinalVoltage){
        *DISCHARGING_STATE = 0;
        endDischarge(DISCHARGING_STATE);
        displayInitial();
    }
    else if(*DISCHARGING_STATE){
        arrangeResistors(*nowVoltage, *operationFinalVoltage, *operationInitialVoltage, *nowCurrent, *batterySelected);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}