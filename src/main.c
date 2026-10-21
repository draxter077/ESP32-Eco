#include "hardware.h"
#include "control.h"
#include "menu.h"

void app_main(void){
    // Inicializa hardware
    // Inicializa LAN
    // Inicializa display

    // Definições de estados e auxiliares
    int CHARGING_STATE = 0, DISCHARGING_STATE = 0;
    float nowCurrent = 0, nowVoltage = 0, nowTemperature = 0;
    float operationFinalVoltage = 0;
    int batterySelected = 0;

    // Display inicial
    while(1){
        // Estado CARGA/DESCARGA
        if(CHARGING_STATE || DISCHARGING_STATE){
            operationCare(&CHARGING_STATE, &DISCHARGING_STATE, &batterySelected, &nowCurrent, &nowVoltage, &nowTemperature, &operationFinalVoltage)
        }
        // Estado MENU
        else{
            if(gpio_get_level(ENTER_BTN)){
                menu(&CHARGING_STATE, &DISCHARGING_STATE, &batterySelected, &nowVoltage, &operationFinalVoltage)
            }
        }
    }
}