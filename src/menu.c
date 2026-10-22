#include "hardware.h"
#include "control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void menu(int* CHARGING_STATE, int* DISCHARGING_STATE, int* batterySelected, float* nowVoltage, float* operationFinalVoltage){
    // Seleciona bateria
    int lastEnterBtnState = 0, actualBtnState = 0;
    while(1){
        *batterySelected = menuSelectBattery();

        // Seleciona a voltagem final
        actualBtnState = gpio_get_level(ENTER_BTN_GPIO);
        if(!actualBtnState && lastEnterBtnState){
            lastEnterBtnState = 0;
            // Atualiza dados de display, calculando porcentagem
            *nowVoltage = calculateVoltage(*batterySelected);
            while(1){
                *operationFinalVoltage = menuSelectFinalVoltage();

                // Confirma operação e começa
                actualBtnState = gpio_get_level(ENTER_BTN_GPIO);
                if(!actualBtnState && lastEnterBtnState && *operationFinalVoltage != *nowVoltage){
                    // Entra no modo de charging/discharging em 5s
                    gpio_set_level(LED_GPIO, 1);
                    // float startTime = 0, nowTime = 0;
                    // while(1){
                    //     if(nowTime - startTime > 5){
                    //         if(*operationFinalVoltage > *nowVoltage){
                    //             startCharge(CHARGING_STATE);
                    //         }
                    //         else if(*operationFinalVoltage < *nowVoltage){
                    //             startDischarge(DISCHARGING_STATE);
                    //         }
                    //         break;
                    //     }
                    //     else if(gpio_get_level(ENTER_BTN_GPIO)){
                    //         // Atualiza display
                    //         *batterySelected = 0;
                    //         break;
                    //     }
                    // }
                    break;
                }
                else{
                    lastEnterBtnState = actualBtnState;
                }

                vTaskDelay(pdMS_TO_TICKS(10));
            }
            break;
        }
        else{
            lastEnterBtnState = actualBtnState;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}