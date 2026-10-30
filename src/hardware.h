#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

//#define EMERGENCY_BTN_GPIO      GPIO_NUM_32
#define EMERGENCY_BTN_GPIO      GPIO_NUM_26

//#define ENTER_BTN_GPIO          GPIO_NUM_4
#define ENTER_BTN_GPIO          GPIO_NUM_33

//#define DG1_GPIO                GPIO_NUM_13 // Libera R paralelo
#define DG1_GPIO                GPIO_NUM_17

//#define DG2_GPIO                GPIO_NUM_14 // Libera segundo série
#define DG2_GPIO                GPIO_NUM_21

//#define DG3_GPIO                GPIO_NUM_15 // Libera terceiro série
#define DG3_GPIO                GPIO_NUM_19


//#define PWM_GPIO    GPIO_NUM_19
#define PWM_GPIO    GPIO_NUM_18

//#define DISCHARGING_GATE_GPIO   GPIO_NUM_17 
#define DISCHARGING_GATE_GPIO   GPIO_NUM_16

//#define LED_GPIO                GPIO_NUM_23 
#define LED_GPIO                GPIO_NUM_25

//#define POTENCIOMETER_GPIO      ADC_CHANNEL_6 
#define POTENCIOMETER_GPIO      ADC_CHANNEL_7 /* adc2 */

// Certo
#define VOLTAGE_GPIO            ADC_CHANNEL_0 /* adc1 */

//#define CURRENT_GPIO            ADC_CHANNEL_7
#define CURRENT_GPIO            ADC_CHANNEL_4 /* adc1 */

#define ADC_UNIT_ID             ADC_UNIT_1
#define ADC_ATTENUATION         ADC_ATTEN_DB_12

extern adc_oneshot_unit_handle_t adc1_handle;
extern adc_oneshot_unit_handle_t adc2_handle;

void hardwareInit(void);

// Retorna a corrente em amperes
float calculateCurrent(void);

// Retorna a tensão em volts
float calculateVoltage(void);

// Seleciona a bateria no display
int menuSelectBattery(void);

// Seleciona a tensão final da bateria no display
float menuSelectFinalVoltage(int batterySelected, float nowVoltage);