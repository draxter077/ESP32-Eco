#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware.h"
#include "display.h"
#include "config.h"
#include "telemetry.h"
#include "esp_timer.h"
#include "driver/ledc.h"
#include <math.h>

#define LOG_INTERVAL_US 100000 // 100 ms

static int64_t lastLogTime = 0;

#define PWM_FREQ_HZ   50000
#define PWM_RES       LEDC_TIMER_10_BIT
#define PWM_MAX       ((1 << 10) - 1)      // 1023
#define PWM_MODE      LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL   LEDC_CHANNEL_0
#define PWM_TIMER     LEDC_TIMER_0
#define SLEW_PER_TICK   0.002f  

static float duty_atual = 0.0f;

void pwm_init(void){
    ledc_timer_config_t timer = {
        .speed_mode      = PWM_MODE,
        .duty_resolution = PWM_RES,
        .timer_num       = PWM_TIMER,
        .freq_hz         = PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .gpio_num   = PWM_GPIO,
        .speed_mode = PWM_MODE,
        .channel    = PWM_CHANNEL,
        .timer_sel  = PWM_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .duty       = 0,          // começa desligado
        .hpoint     = 0
    };
    ledc_channel_config(&ch);
}

void pwm_stop(){
    ledc_set_duty(PWM_MODE, PWM_CHANNEL, 0);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}

void emergencyValve(){
    gpio_set_level(DISCHARGING_GATE_GPIO, 0);
    pwm_stop();
    gpio_set_level(DG1_GPIO, 0);
    gpio_set_level(DG2_GPIO, 0);
    gpio_set_level(DG3_GPIO, 0);
    gpio_set_level(LED_GPIO, 1);
    displayEmergency();
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(LED_GPIO, 0);
}


void startCharge(int* CHARGING_STATE){
    if(*CHARGING_STATE == 1){
        return;
    }
    *CHARGING_STATE = 1;
    pwm_stop();
    gpio_set_level(DISCHARGING_GATE_GPIO, 0);
    lastLogTime = esp_timer_get_time();
}

void endCharge(int* CHARGING_STATE){
    *CHARGING_STATE = 0;
    pwm_stop();
    flushBufferToFlash();
}

void startDischarge(int* DISCHARGING_STATE){
    if(*DISCHARGING_STATE == 1){
        return;
    }
    *DISCHARGING_STATE = 1;
    gpio_set_level(DISCHARGING_GATE_GPIO, 1);
    pwm_stop();
    lastLogTime = esp_timer_get_time();
}

void endDischarge(int* DISCHARGING_STATE){

    *DISCHARGING_STATE = 0;
    gpio_set_level(DISCHARGING_GATE_GPIO, 0);
    gpio_set_level(DG1_GPIO, 0);
    gpio_set_level(DG2_GPIO, 0);
    gpio_set_level(DG3_GPIO, 0);
    flushBufferToFlash();
}

void arrangeResistors(float nowVoltage){
    if(nowVoltage > 30){
        gpio_set_level(DG1_GPIO, 0);
        gpio_set_level(DG2_GPIO, 1);
        gpio_set_level(DG3_GPIO, 1);
    }
    else if(nowVoltage > 14){
        gpio_set_level(DG1_GPIO, 0);
        gpio_set_level(DG2_GPIO, 1);
        gpio_set_level(DG3_GPIO, 0);
    }
    else if(nowVoltage > 10){
        gpio_set_level(DG1_GPIO, 0);
        gpio_set_level(DG2_GPIO, 0);
        gpio_set_level(DG3_GPIO, 0);
    }
    else if(nowVoltage > 6){
        gpio_set_level(DG1_GPIO, 1);
        gpio_set_level(DG2_GPIO, 1);
        gpio_set_level(DG3_GPIO, 0);
    }
    else{
        gpio_set_level(DG1_GPIO, 1);
        gpio_set_level(DG2_GPIO, 0);
        gpio_set_level(DG3_GPIO, 0);
    }
}


// Retorna a proporção de tempo que o mosfet de carga deve ficar ligado 
// Retorna entre 0 e 1
float calculatePWM(batteryData bat, float V_bat, float I_bat){

    float V_target = fmin(V_bat + V_OFFSET, bat.maxVoltage);
    float duty = V_target/42.0;
    
    return duty;
}

void pwm_write(float d){

    if (d < 0.0f){
        d = 0.0f;
    }
    if (d > 1.0f){
        d = 1.0f;
    } 

    uint32_t valor = (uint32_t)(d * PWM_MAX + 0.5f);
    ledc_set_duty(PWM_MODE, PWM_CHANNEL, valor);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}

void pwm_set_duty(batteryData bat, float V_bat, float I_bat){
 
    float d = calculatePWM(bat, V_bat, I_bat);
 
    if (d < 0.0f)  d = 0.0f;
    if (d > 1) d = 1;
 

    if (d > duty_atual + SLEW_PER_TICK) d = duty_atual + SLEW_PER_TICK;
 
    duty_atual = d;
    pwm_write(duty_atual);
}


void operationCare(int* CHARGING_STATE, int* DISCHARGING_STATE, int* batterySelected, float* nowCurrent, float* nowVoltage, float* operationFinalVoltage, float* operationInitialVoltage, int64_t* operationStartTime, int64_t* lastTimeError, int64_t* lastTimeFinish){

    batteryData b = batteries[*batterySelected];

    int64_t currentTime = esp_timer_get_time();
    if (currentTime - lastLogTime >= LOG_INTERVAL_US) {
        saveLogRecord(currentTime,*nowCurrent,*nowVoltage);
        lastLogTime = currentTime;
        displayInOperation(*CHARGING_STATE, *DISCHARGING_STATE, *operationInitialVoltage, *operationFinalVoltage, *nowVoltage, *nowCurrent, *batterySelected);
    }

    if(gpio_get_level(EMERGENCY_BTN_GPIO) || *nowVoltage > b.maxVoltage*1.05 || *nowCurrent > 3){
        if(esp_timer_get_time() - *lastTimeError > 10*1000*1000) *lastTimeError = esp_timer_get_time();
        if(esp_timer_get_time() - *lastTimeError > 5*1000*1000 || gpio_get_level(EMERGENCY_BTN_GPIO)){
            *CHARGING_STATE = 0;
            *DISCHARGING_STATE = 0;
            emergencyValve();
            displayInitial();
        }
    }
    
    if((*CHARGING_STATE && *nowVoltage >= *operationFinalVoltage) || (*DISCHARGING_STATE && *nowVoltage <= *operationFinalVoltage)){
        if(esp_timer_get_time() - *lastTimeFinish > 10*1000*1000) *lastTimeFinish = esp_timer_get_time();
        else if(esp_timer_get_time() - *lastTimeFinish > 5*1000*1000){
            if(*CHARGING_STATE) endCharge(CHARGING_STATE);
            else if(*DISCHARGING_STATE) endDischarge(DISCHARGING_STATE);
            displayInitial();
        }
    }

    if(batterySelected >= 0 && batterySelected < 2){
        if(*DISCHARGING_STATE){
            arrangeResistors(*nowVoltage);
        }
        else if(*CHARGING_STATE){
                    pwm_set_duty(b, *nowVoltage, *nowCurrent);
                }
    }
    
}