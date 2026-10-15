/*
Quem roda e quando?
*/

// teste

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define EMERGENCY_BTN           GPIO_NUM_
#define SELECT_CHARGING_BTN     GPIO_NUM_
#define SELECT_DISCHARGING_BTN  GPIO_NUM_
#define RL1_GPIO                GPIO_NUM_
#define RL2_GPIO                GPIO_NUM_
#define RL3_GPIO                GPIO_NUM_
#define RL4_GPIO                GPIO_NUM_
#define RL5_GPIO                GPIO_NUM_
#define FAN_GPIO                GPIO_NUM_

#define ADC_UNIT_ID             ADC_UNIT_1
#define VOLTAGE_GPIO            ADC_CHANNEL_6
#define CURRENT_GPIO            ADC_CHANNEL_7
#define ADC_ATTENUATION         ADC_ATTEN_DB_12

static adc_oneshot_unit_handle_t adc1_handle = NULL;

float calculateCurrent(){
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, CURRENT_GPIO, &raw));    
    return ((raw/4095.0)*3.3/10.0)/0.1;
}

float calculateVoltage(){
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, VOLTAGE_GPIO, &raw));
    return ((raw/4095.0)*3.3)*(39.0+3.3)/3.3;
}
void saveTelemetry(float current, float voltage){}

void app_main(){
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