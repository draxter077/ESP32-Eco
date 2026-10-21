#include "driver/gpio.h"

#define EMERGENCY_BTN           GPIO_NUM_1
#define ENTER_BTN               GPIO_NUM_1
#define RL1_GPIO                GPIO_NUM_1
#define RL2_GPIO                GPIO_NUM_1
#define RL3_GPIO                GPIO_NUM_1
#define RL4_GPIO                GPIO_NUM_1
#define RL5_GPIO                GPIO_NUM_1
#define FAN_GPIO                GPIO_NUM_1
#define LED_GPIO                GPIO_NUM_1

#define VOLTAGE_GPIO            ADC_CHANNEL_6
#define CURRENT_GPIO            ADC_CHANNEL_7
#define TEMPERATURE_GPIO        ADC_CHANNEL_8

float calculateCurrent(void);
float calculateVoltage(int batterySelected);
float calculateTemperature(void);
int menuSelectBattery(void);
int menuSelectFinalVoltage(void);