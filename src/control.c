#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware.h"
#include "display.h"
#include "config.h"
#include "esp_timer.h"

void emergencyValve(char* emergencyText){
    gpio_set_level(GATE1_GPIO, 0);
    gpio_set_level(GATE2_GPIO, 0);
    gpio_set_level(GATE3_GPIO, 0);
    gpio_set_level(GATE4_GPIO, 0);
    gpio_set_level(GATE5_GPIO, 0);
    gpio_set_level(FAN_GPIO, 0);
    gpio_set_level(LED_GPIO, 1);
    displayInOperation(0,0,0,0,0,0,0,emergencyText);
    vTaskDelay(pdMS_TO_TICKS(5000));
}

void startCharge(int* CHARGING_STATE){
    *CHARGING_STATE = 1;
    gpio_set_level(GATE2_GPIO, 1);
}

void endCharge(void){
    gpio_set_level(GATE2_GPIO, 0);
}

void startDischarge(int* DISCHARGING_STATE){
    *DISCHARGING_STATE = 1;
    gpio_set_level(GATE1_GPIO, 1);
    gpio_set_level(FAN_GPIO, 1);
}

void endDischarge(void){
    gpio_set_level(GATE1_GPIO, 0);
    gpio_set_level(GATE3_GPIO, 0);
    gpio_set_level(GATE4_GPIO, 0);
    gpio_set_level(GATE5_GPIO, 0);
    gpio_set_level(FAN_GPIO, 0);
}

void arrangeResistors(float nowVoltage, float operationFinalVoltage, float operationInitialVoltage, float nowCurrent, int batterySelected){
    if(nowVoltage > 28){
        gpio_set_level(GATE3_GPIO, 1);
        gpio_set_level(GATE4_GPIO, 0);
        gpio_set_level(GATE5_GPIO, 1);
        displayInOperation(0,1,operationInitialVoltage,operationFinalVoltage,nowVoltage,nowCurrent,batterySelected,"R>28");
    }
    else if(nowVoltage > 15){
        gpio_set_level(GATE3_GPIO, 1);
        gpio_set_level(GATE4_GPIO, 0);
        gpio_set_level(GATE5_GPIO, 0);
        displayInOperation(0,1,operationInitialVoltage,operationFinalVoltage,nowVoltage,nowCurrent,batterySelected,"R>15");
    }
    else if(nowVoltage > 6){
        gpio_set_level(GATE3_GPIO, 0);
        gpio_set_level(GATE4_GPIO, 0);
        gpio_set_level(GATE5_GPIO, 0);
        displayInOperation(0,1,operationInitialVoltage,operationFinalVoltage,nowVoltage,nowCurrent,batterySelected,"R>6");
    }
    else{
        gpio_set_level(GATE3_GPIO, 0);
        gpio_set_level(GATE4_GPIO, 1);
        gpio_set_level(GATE5_GPIO, 0);
        displayInOperation(0,1,operationInitialVoltage,operationFinalVoltage,nowVoltage,nowCurrent,batterySelected,"R<=6");
    }
}

void operationCare(int* CHARGING_STATE, int* DISCHARGING_STATE, int* batterySelected, float* nowCurrent, float* nowVoltage, float* operationFinalVoltage, float* operationInitialVoltage, int64_t* lastTimeError){
    *nowCurrent = calculateCurrent();
    *nowVoltage = calculateVoltage(*batterySelected);
    batteryData b = batteries[*batterySelected];
    // Salva telemetria

    displayInOperation(*CHARGING_STATE, *DISCHARGING_STATE, *operationInitialVoltage, *operationFinalVoltage, *nowVoltage, *nowCurrent, *batterySelected, "...");
    if(gpio_get_level(EMERGENCY_BTN_GPIO) || ((*nowVoltage > b.maxVoltage*0.95 || *nowCurrent > 3) && esp_timer_get_time() - *lastTimeError > 5*1000*1000 && esp_timer_get_time() - *lastTimeError < 10*1000*1000)){
        *CHARGING_STATE = 0;
        *DISCHARGING_STATE = 0;
        emergencyValve("Teste");
        displayInitial();
    }
    else if(esp_timer_get_time() - *lastTimeError > 10*1000*1000 && *lastTimeError){
        *lastTimeError = 0;
    }
    else if((*nowVoltage > b.maxVoltage*0.95 || *nowCurrent > 3) && !(*lastTimeError)){
        *lastTimeError = esp_timer_get_time();
    }
    
    if(*CHARGING_STATE && *nowVoltage >= *operationFinalVoltage){
        *CHARGING_STATE = 0;
        endCharge();
        displayInitial();
    }
    else if(*DISCHARGING_STATE && *nowVoltage <= *operationFinalVoltage){
        *DISCHARGING_STATE = 0;
        endDischarge();
        displayInitial();
    }
    else if(*DISCHARGING_STATE){
        arrangeResistors(*nowVoltage, *operationFinalVoltage, *operationInitialVoltage, *nowCurrent, *batterySelected);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}