#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define EMERGENCY_BTN_GPIO      GPIO_NUM_32
#define ENTER_BTN_GPIO          GPIO_NUM_4
#define DG1_GPIO                GPIO_NUM_13 // Libera R paralelo
#define DG2_GPIO                GPIO_NUM_14 // Libera segundo série
#define DG3_GPIO                GPIO_NUM_15 // Libera terceiro série
#define CHARGING_42_GATE_GPIO   GPIO_NUM_19 /* Lê no pino 10 */

#define DISCHARGING_GATE_GPIO   GPIO_NUM_17 /* Pino 7 */
#define LED_GPIO                GPIO_NUM_23 /* Pino 9 */

#define POTENCIOMETER_GPIO      ADC_CHANNEL_6 /* Pino 5 */
#define VOLTAGE_GPIO            ADC_CHANNEL_0 /* Lê tensão no pino 17 */
#define CURRENT_GPIO            ADC_CHANNEL_7 /* Lê corrente no pino 20 */

#define ADC_UNIT_ID             ADC_UNIT_1
#define ADC_ATTENUATION         ADC_ATTEN_DB_12

extern adc_oneshot_unit_handle_t adc1_handle;

void hardwareInit(void);

// Retorna a corrente em amperes
float calculateCurrent(void);

// Retorna a tensão em volts
float calculateVoltage(void);

// Seleciona a bateria no display
int menuSelectBattery(void);

// Seleciona a tensão final da bateria no display
int menuSelectFinalVoltage(int batterySelected, float nowVoltage);