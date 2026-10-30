#include "hardware.h"
#include "display.h"
#include "config.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

adc_cali_handle_t adc1_cali_handle = NULL;
static esp_err_t initADCCalibration(void)
{
    adc_cali_line_fitting_config_t config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    return adc_cali_create_scheme_line_fitting(&config,&adc1_cali_handle);
}

adc_oneshot_unit_handle_t adc1_handle = NULL;
batteryData batteries[2] = {
    [0] = {"Nancy 42V", 42.0f, 30.0f},
    [1] = {"Aveiro 12V", 12.6f, 9.0f} 
};

/* Corrigir mosfet de carga e FAN*/
void hardwareInit(void){
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL<<DISCHARGING_GATE_GPIO) | (1ULL<<CHARGING_42_GATE_GPIO) | 
                        (1ULL<<DG1_GPIO) | (1ULL<<DG2_GPIO) | (1ULL<<DG3_GPIO) |
                        (1ULL<<LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_conf);

    gpio_set_level(CHARGING_42_GATE_GPIO, 0);
    gpio_set_level(DISCHARGING_GATE_GPIO, 0);
    gpio_set_level(DG1_GPIO, 0);
    gpio_set_level(DG2_GPIO, 0);
    gpio_set_level(DG3_GPIO, 0);
    gpio_set_level(LED_GPIO, 0);

    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL<<EMERGENCY_BTN_GPIO) | (1ULL<<ENTER_BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&in_conf);

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_ID,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTENUATION,
    };
    adc_oneshot_config_channel(adc1_handle, VOLTAGE_GPIO, &adc_config);
    adc_oneshot_config_channel(adc1_handle, CURRENT_GPIO, &adc_config);
    adc_oneshot_config_channel(adc1_handle, POTENCIOMETER_GPIO, &adc_config);

    esp_err_t err = initADCCalibration();
    if (err != ESP_OK) {
        printf("Falha na calibracao do ADC");
        exit(1);
    }

    displayInit();
}

static esp_err_t readAdcAverage(adc_channel_t channel, int samples, float *average)
{
    if (adc1_handle == NULL || average == NULL || samples <= 0) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw = 0;
    int64_t sum = 0;

    for (int i = 0; i < samples; ++i) {
        esp_err_t err = adc_oneshot_read(adc1_handle, channel, &raw);
        if (err != ESP_OK) {
            return err;
        }
        sum += raw;
    }

    *average = (float)sum / (float)samples;
    return ESP_OK;
}

#define WCS_ZERO 2.50f
#define WCS_SENSITIVITY 0.066f
#define QTD_AMOSTRAS 5


float calculateCurrent(void)
{
    int raw = 0;
    int adc_mV = 0;
    int soma_mV = 0;


    for (int i = 0; i < QTD_AMOSTRAS; i++) {
        esp_err_t err = adc_oneshot_read(adc1_handle,CURRENT_GPIO,&raw);

        if (err != ESP_OK) {
            return -1.0f;
        }

        // Converte cada leitura para milivolts 
        err = adc_cali_raw_to_voltage(adc1_cali_handle,raw,&adc_mV);

        if (err != ESP_OK) {
            return -1.0f;
        }

        soma_mV += adc_mV;
    }

    // Tensão média no pino ADC
    float tensaoADC = (soma_mV / (float)QTD_AMOSTRAS) / 1000.0f;

    // Recupera a tensão original na saída do WCS1800
    float tensaoSensor = tensaoADC * 1.5f;

    // Converte a tensão do sensor em corrente
    float corrente = (tensaoSensor - WCS_ZERO) / WCS_SENSITIVITY;

    return corrente;
}


float calculateVoltage(void){

    float rawAverage = 0.0f;

    if (readAdcAverage(VOLTAGE_GPIO,QTD_AMOSTRAS, &rawAverage) != ESP_OK) {
        return -1.0f;
    }

    const float adcVoltage = (rawAverage / 4095.0f) * 3.3f;

    const float batteryVoltage = adcVoltage * (212.0f/12.0f);

    return batteryVoltage;
}

int menuSelectBattery(void){
    int raw = 0;
    esp_err_t err = adc_oneshot_read(adc1_handle,POTENCIOMETER_GPIO,&raw);

    if (err != ESP_OK){
        return -1;
    }

    int readValue = (raw >= 2048) ? 1 : 0;
    displayBatterySelect(readValue);

    return readValue;
}

float menuSelectFinalVoltage(int batterySelected, float nowVoltage){
    if (batterySelected < 0 || batterySelected >= 2 || adc1_handle == NULL) {
        return NAN;
    }

    int raw = 0;
    if (adc_oneshot_read(adc1_handle, POTENCIOMETER_GPIO, &raw) != ESP_OK) {
        return NAN;
    }

    const batteryData battery = batteries[batterySelected];
    const float ratio = (float)raw / 4095.0f;
    float selectedVoltage = battery.minVoltage +
        (battery.maxVoltage - battery.minVoltage) * ratio;

    if (selectedVoltage < battery.minVoltage){
        selectedVoltage = battery.minVoltage;
    }
    if (selectedVoltage > battery.maxVoltage){ 
        selectedVoltage = battery.maxVoltage;
    }
    // No display o valor aparece arredondado, porém é float
    displayFinalVoltageSelect((int)lroundf(selectedVoltage), batterySelected, nowVoltage);
    return selectedVoltage;
}
