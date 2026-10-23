#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define EMERGENCY_BTN_GPIO      GPIO_NUM_32
#define ENTER_BTN_GPIO          GPIO_NUM_4
#define DG1_GPIO                GPIO_NUM_13 // Libera R paralelo
#define DG2_GPIO                GPIO_NUM_14 // Libera segundo série
#define DG3_GPIO                GPIO_NUM_15 // Libera terceiro série
#define CHARGING_42_GATE_GPIO   GPIO_NUM_19
#define CHARGING_12_GATE_GPIO   GPIO_NUM_25
#define DISCHARGING_GATE_GPIO   GPIO_NUM_17
#define FAN_GPIO                GPIO_NUM_18
#define LED_GPIO                GPIO_NUM_23

#define POTENCIOMETER_GPIO      ADC_CHANNEL_6 // GPIO34
#define VOLTAGE_GPIO            ADC_CHANNEL_0 // GPIOVP/A0
#define CURRENT_GPIO            ADC_CHANNEL_7 // GPIO27

#define ADC_UNIT_ID             ADC_UNIT_1
#define ADC_ATTENUATION         ADC_ATTEN_DB_12

extern adc_oneshot_unit_handle_t adc1_handle;

void hardwareInit(void);
float calculateCurrent(void);
float calculateVoltage(void);
float calculateTemperature(void);
int menuSelectBattery(void);
int menuSelectFinalVoltage(int batterySelected, float nowVoltage);