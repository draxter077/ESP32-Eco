#include "hardware.h"
#include "control.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

void menu(int* CHARGING_STATE, int* DISCHARGING_STATE, int* batterySelected, float* nowVoltage, float* operationFinalVoltage, float* operationInitialVoltage, int64_t* operationStartTime){
    int lastEnterBtnState = 0, actualBtnState = 0;
    int64_t startMenuTime = esp_timer_get_time();
    while(1){
        *batterySelected = menuSelectBattery();
        actualBtnState = gpio_get_level(ENTER_BTN_GPIO);
        if(!actualBtnState && lastEnterBtnState){
            lastEnterBtnState = 0;
            *nowVoltage = calculateVoltage();
            *operationInitialVoltage = *nowVoltage;
            startMenuTime = esp_timer_get_time();
            while(1){
                *operationFinalVoltage = menuSelectFinalVoltage(*batterySelected, *nowVoltage);
                actualBtnState = gpio_get_level(ENTER_BTN_GPIO);
                if(!actualBtnState && lastEnterBtnState && *operationFinalVoltage != *nowVoltage){
                    lastEnterBtnState = 0;
                    startMenuTime = esp_timer_get_time();
                    displayPreOperation();
                    while(1){
                        actualBtnState = gpio_get_level(ENTER_BTN_GPIO);
                        if(esp_timer_get_time() - startMenuTime > 5*1000*1000){
                            *operationStartTime = esp_timer_get_time();
                            if(*operationFinalVoltage > *nowVoltage){
                                int TYPE = 0;
                                if(*batterySelected == 0) TYPE = 42;
                                else if(*batterySelected == 1) TYPE = 12;
                                startCharge(CHARGING_STATE, TYPE);
                            }
                            else if(*operationFinalVoltage < *nowVoltage){
                                startDischarge(DISCHARGING_STATE);
                            }
                            break;
                        }
                        else if(!actualBtnState && lastEnterBtnState){
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
                else if(esp_timer_get_time() - startMenuTime > 20*1000*1000){
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
        else if(esp_timer_get_time() - startMenuTime > 20*1000*1000){
            displayInitial();
            break;
        }
        else{
            lastEnterBtnState = actualBtnState;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}