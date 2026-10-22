#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define EMERGENCY_BTN_GPIO      GPIO_NUM_32
#define ENTER_BTN_GPIO          GPIO_NUM_4
#define GATE1_GPIO              GPIO_NUM_13
#define GATE2_GPIO              GPIO_NUM_14
#define GATE3_GPIO              GPIO_NUM_15
#define GATE4_GPIO              GPIO_NUM_16
#define GATE5_GPIO              GPIO_NUM_17
#define FAN_GPIO                GPIO_NUM_18
#define LED_GPIO                GPIO_NUM_23

#define POTENCIOMETER_GPIO      ADC_CHANNEL_6
#define VOLTAGE_GPIO            ADC_CHANNEL_0
#define CURRENT_GPIO            ADC_CHANNEL_7

#define ADC_UNIT_ID             ADC_UNIT_1
#define ADC_ATTENUATION         ADC_ATTEN_DB_12

extern adc_oneshot_unit_handle_t adc1_handle;

void hardwareInit(void);
float calculateCurrent(void);
float calculateVoltage(int batterySelected);
float calculateTemperature(void);
int menuSelectBattery(void);
int menuSelectFinalVoltage(int batterySelected, float nowVoltage);