#include "hardware.h"
#include "control.h"
#include "menu.h"
#include "display.h"
#include "network.h"
#include "telemetry.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "main.h"
#include "nvs_flash.h"

int CHARGING_STATE = 0, DISCHARGING_STATE = 0;
float nowCurrent = 0, nowVoltage = 0;
float operationFinalVoltage = 0, operationInitialVoltage = 0;
int batterySelected = 0;
int lastEnterBtnState = 0, actualBtnState = 0;
int64_t lastErrorTime = 0, lastTimeFinish = 0, operationStartTime = 0;

void app_main(void){
    hardwareInit();
    nvs_flash_init();
    initStorage();
    wifi_init_softap();
    start_webserver();
    displayInitial();

    while(1){
        if(CHARGING_STATE || DISCHARGING_STATE){
            operationCare(&CHARGING_STATE, &DISCHARGING_STATE, &batterySelected, &nowCurrent, &nowVoltage, &operationFinalVoltage, &operationInitialVoltage, &operationStartTime, &lastErrorTime, &lastTimeFinish);
        }
        else{
            actualBtnState = gpio_get_level(ENTER_BTN_GPIO);
            if(!actualBtnState && lastEnterBtnState){
                menu(&CHARGING_STATE, &DISCHARGING_STATE, &batterySelected, &nowVoltage, &operationFinalVoltage, &operationInitialVoltage, &operationStartTime);
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