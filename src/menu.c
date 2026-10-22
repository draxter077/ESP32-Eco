#include "hardware.h"
#include "control.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

void menu(int* CHARGING_STATE, int* DISCHARGING_STATE, int* batterySelected, float* nowVoltage, float* operationFinalVoltage, float* operationInitialVoltage){
    int lastEnterBtnState = 0, actualBtnState = 0;
    int64_t startMenuTime = esp_timer_get_time();
    while(1){
        *batterySelected = menuSelectBattery();
        actualBtnState = gpio_get_level(ENTER_BTN_GPIO);
        if(!actualBtnState && lastEnterBtnState){
            lastEnterBtnState = 0;
            *nowVoltage = calculateVoltage(*batterySelected);
            *operationInitialVoltage = *nowVoltage;
            startMenuTime = esp_timer_get_time();
            while(1){
                *operationFinalVoltage = menuSelectFinalVoltage(*batterySelected);
                actualBtnState = gpio_get_level(ENTER_BTN_GPIO);
                if(!actualBtnState && lastEnterBtnState && *operationFinalVoltage != *nowVoltage){
                    startMenuTime = esp_timer_get_time();
                    while(1){
                        if(esp_timer_get_time() - startMenuTime > 5*1000*1000){
                            if(*operationFinalVoltage > *nowVoltage){
                                startCharge(CHARGING_STATE);
                            }
                            else if(*operationFinalVoltage < *nowVoltage){
                                startDischarge(DISCHARGING_STATE);
                            }
                            break;
                        }
                        else if(gpio_get_level(ENTER_BTN_GPIO)){
                            displayInitial();
                            break;
                        }

                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                    break;
                }
                else if(esp_timer_get_time() - startMenuTime > 5*1000*1000){
                    displayInitial();
                    break;
                }
                else{
                    lastEnterBtnState = actualBtnState;
                }

                vTaskDelay(pdMS_TO_TICKS(10));
            }
            break;
        }
        else if(esp_timer_get_time() - startMenuTime > 5*1000*1000){
            displayInitial();
            break;
        }
        else{
            lastEnterBtnState = actualBtnState;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}