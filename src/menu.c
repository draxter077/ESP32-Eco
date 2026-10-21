#include "hardware.h"

void menu(int* CHARGING_STATE, int* DISCHARGING_STATE, int* batterySelected, float* nowVoltage, float* operationFinalVoltage){
    // Atualizar display para selecionar bateria

    // Seleciona bateria
    while(1){
        *batterySelected = menuSelectBattery();

        // Seleciona a voltagem final
        if(gpio_get_level(ENTER_BTN)){
            // Atualiza dados de display, calculando porcentagem
            *nowVoltage = calculateVoltage(*batterySelected);
            while(1){
                *operationFinalVoltage = menuSelectFinalVoltage();

                // Confirma operação e começa
                if(gpio_get_level(ENTER_BTN) && *operationFinalVoltage != *nowVoltage){
                    // Entra no modo de charging/discharging em 5s
                    float startTime = 0, nowTime = 0;
                    while(1){
                        if(nowTime - startTime > 5){
                            if(*operationFinalVoltage > *nowVoltage){
                                *CHARGING_STATE = 1;
                                // Atualiza display
                            }
                            else if(*operationFinalVoltage < *nowVoltage){
                                *DISCHARGING_STATE = 1;
                                // Atualiza display
                            }
                            break;
                        }
                        else if(gpio_get_level(ENTER_BTN)){
                            // Atualiza display
                            *batterySelected = 0;
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
}