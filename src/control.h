#include "config.h"

void pwm_init(void);
void pwm_stop();
void pwm_set_duty(batteryData bat, float V_bat, float I_bat);
void operationCare(int* CHARGING_STATE, int* DISCHARGING_STATE, int* batterySelected, float* nowCurrent, float* nowVoltage, float* operationFinalVoltage, float* operationInitialVoltage, int64_t* operationStartTime, int64_t* lastTimeError, int64_t* lastTimeFinish);
void startCharge(int* CHARGING_STATE, int TYPE);
void startDischarge(int* DISCHARGING_STATE);
void emergencyValve(void);