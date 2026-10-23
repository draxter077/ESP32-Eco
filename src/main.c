#include "hardware.h"
#include "control.h"
#include "menu.h"
#include "display.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

void app_main(void){
    // Inicializa hardware
    hardwareInit();

    // Definicoes de estados e auxiliares
    int CHARGING_STATE = 0, DISCHARGING_STATE = 0;
    float nowCurrent = 0, nowVoltage = 0;
    float operationFinalVoltage = 0, operationInitialVoltage = 0;
    int batterySelected = 0;
    int lastEnterBtnState = 0, actualBtnState = 0;
    int64_t lastErrorTime = 0;

    displayInitial();
    while(1){
        if(CHARGING_STATE || DISCHARGING_STATE){
            operationCare(&CHARGING_STATE, &DISCHARGING_STATE, &batterySelected, &nowCurrent, &nowVoltage, &operationFinalVoltage, &operationInitialVoltage, &lastErrorTime);
        }
        else{
            actualBtnState = gpio_get_level(ENTER_BTN_GPIO);
            if(!actualBtnState && lastEnterBtnState){
                menu(&CHARGING_STATE, &DISCHARGING_STATE, &batterySelected, &nowVoltage, &operationFinalVoltage, &operationInitialVoltage);
                lastEnterBtnState = 0;
                displayInitial();
            }
            else{
                lastEnterBtnState = actualBtnState;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}