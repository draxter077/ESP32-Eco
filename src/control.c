#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware.h"

void emergencyValve(char* emergencyText){
    gpio_set_level(GATE1_GPIO, 0);
    gpio_set_level(GATE2_GPIO, 0);
    gpio_set_level(GATE3_GPIO, 0);
    gpio_set_level(GATE4_GPIO, 0);
    gpio_set_level(GATE5_GPIO, 0);
    gpio_set_level(FAN_GPIO, 0);

    // Atualizar texto no display
    // while(1){} para LED piscando, aguardando pressionar botão
}

void startCharge(int* CHARGING_STATE){
    *CHARGING_STATE = 1;
    gpio_set_level(GATE2_GPIO, 1);
    // Atualiza display
}

void endCharge(void){
    gpio_set_level(GATE2_GPIO, 0);

    // Atualizar texto no display
}

void startDischarge(int* DISCHARGING_STATE){
    *DISCHARGING_STATE = 1;
    gpio_set_level(GATE1_GPIO, 1);
    gpio_set_level(FAN_GPIO, 1);
    // Atualiza display
}

void endDischarge(void){
    gpio_set_level(GATE1_GPIO, 0);
    gpio_set_level(GATE3_GPIO, 0);
    gpio_set_level(GATE4_GPIO, 0);
    gpio_set_level(GATE5_GPIO, 0);
    gpio_set_level(FAN_GPIO, 0);

    // Atualizar texto no display
}

void arrangeResistors(float nowVoltage){
    if(nowVoltage > 28){
        gpio_set_level(GATE3_GPIO, 1);
        gpio_set_level(GATE4_GPIO, 0);
        gpio_set_level(GATE5_GPIO, 1);

        // Atualizar texto no display
    }
    else if(nowVoltage > 15){
        gpio_set_level(GATE3_GPIO, 1);
        gpio_set_level(GATE4_GPIO, 0);
        gpio_set_level(GATE5_GPIO, 0);

        // Atualizar texto no display
    }
    else if(nowVoltage > 6){
        gpio_set_level(GATE3_GPIO, 0);
        gpio_set_level(GATE4_GPIO, 0);
        gpio_set_level(GATE5_GPIO, 0);

        // Atualizar texto no display
    }
    else{
        gpio_set_level(GATE3_GPIO, 0);
        gpio_set_level(GATE4_GPIO, 1);
        gpio_set_level(GATE5_GPIO, 0);

        // Atualizar texto no display
    }
}

void operationCare(int* CHARGING_STATE, int* DISCHARGING_STATE, int* batterySelected, float* nowCurrent, float* nowVoltage, float* nowTemperature, float* operationFinalVoltage){
    *nowCurrent = calculateCurrent();
    *nowVoltage = calculateVoltage(*batterySelected);
    *nowTemperature = calculateTemperature();
    // Salva telemetria

    if(gpio_get_level(EMERGENCY_BTN_GPIO) || *nowVoltage > 45 || *nowCurrent > 3){
        *CHARGING_STATE = 0;
        *DISCHARGING_STATE = 0;
        emergencyValve("Teste");
        // Voltar display para inicial
    }
    else if(*CHARGING_STATE && *nowVoltage >= *operationFinalVoltage){
        *CHARGING_STATE = 0;
        endCharge();
        // Voltar display para inicial
    }
    else if(*DISCHARGING_STATE && *nowVoltage <= *operationFinalVoltage){
        *DISCHARGING_STATE = 0;
        endDischarge();
        // Voltar display para inicial
    }
    else if(*DISCHARGING_STATE){
        arrangeResistors(*nowVoltage);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}