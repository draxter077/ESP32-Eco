#include "hardware.h"

void app_main(void){
    hardware_init();

    while(1){}
}

void DEPRECATED_app_main(){
    int CHARGING_STATE = 0, DISCHARGING_STATE = 0;
    float current = 0, voltage = 0;
    float operationFinalVoltage = -1;

    // Configurações da LAN

    // Medição para porcentgaem da bateria
    voltage = calculateVoltage();
    while(1){ 
        // Operações padrão de estado
        if(CHARGING_STATE || DISCHARGING_STATE){
            // Medições
            current = calculateCurrent();
            voltage = calculateVoltage();
            saveTelemetry(current,voltage);

            // Gestão da emergência (colocar caso de excesso de tempratura)
            if(gpio_get_level(EMERGENCY_BTN) || voltage > 45 || current > 3){
                CHARGING_STATE = 0;
                DISCHARGING_STATE = 0;
                gpio_set_level(RL1_GPIO, 0);
                gpio_set_level(RL2_GPIO, 0);
                gpio_set_level(RL3_GPIO, 0);
                gpio_set_level(RL4_GPIO, 0);
                gpio_set_level(RL5_GPIO, 0);
                gpio_set_level(FAN_GPIO, 0);
                operationFinalVoltage = -1;
                // Aviso no Display
                // Aviso no site
                while(1){
                    // Piscar LED 
                }
            }

            // Gestão de final de operação
            if(CHARGING_STATE && voltage >= operationFinalVoltage){
                CHARGING_STATE = 0;
                gpio_set_level(RL2_GPIO, 0);
                operationFinalVoltage = -1;
            }
            else if(DISCHARGING_STATE && voltage <= operationFinalVoltage){
                DISCHARGING_STATE = 0;
                gpio_set_level(RL1_GPIO, 0);
                gpio_set_level(RL3_GPIO, 0);
                gpio_set_level(RL4_GPIO, 0);
                gpio_set_level(RL5_GPIO, 0);
                gpio_set_level(FAN_GPIO, 0);
                operationFinalVoltage = -1;
            }

            // DESCARGA: Gestão do banco de resistores
            if(DISCHARGING_STATE){
                if(voltage > 28){
                    gpio_set_level(RL3_GPIO, 1);
                    gpio_set_level(RL4_GPIO, 0);
                    gpio_set_level(RL5_GPIO, 1);
                }
                else if(voltage > 15){
                    gpio_set_level(RL3_GPIO, 1);
                    gpio_set_level(RL4_GPIO, 0);
                    gpio_set_level(RL5_GPIO, 0);
                }
                else if(voltage > 6){
                    gpio_set_level(RL3_GPIO, 0);
                    gpio_set_level(RL4_GPIO, 0);
                    gpio_set_level(RL5_GPIO, 0);
                }
                else{
                    gpio_set_level(RL3_GPIO, 0);
                    gpio_set_level(RL4_GPIO, 1);
                    gpio_set_level(RL5_GPIO, 0);
                }
            }

            // Atualiza valores no display
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else{
            if(gpio_get_level(SELECT_CHARGING_BTN) && operationFinalVoltage != -1){
                CHARGING_STATE = 1;
                gpio_set_level(RL2_GPIO, 1);
            }
            else if(gpio_get_level(SELECT_DISCHARGING_BTN) && operationFinalVoltage != -1){
                DISCHARGING_STATE = 1;
                gpio_set_level(RL1_GPIO, 1);
                gpio_set_level(FAN_GPIO, 1);
            }

            // Lê potenciômetro
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}