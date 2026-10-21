#include "ssd1306.h"
#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO    22      // Pino SCL conectado ao display
#define I2C_MASTER_SDA_IO    21      // Pino SDA conectado ao display
#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_FREQ_HZ   400000  // 400kHz (Velocidade do I2C)

void display_init(void){
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    
    ssd1306_init(); 
}

void display_update(int state, int battery_soc, float target_voltage, const char* op_type, int progress_percent, int time_remaining_sec, float current_v, float current_i){ // 128x32
    ssd1306_clear();

    // display selecionar bateria [variar conforme potenciometro]
    // display selecionar voltagem final [variar conforme potenciometro]
    // display confirmar operação [atualizar o tempo]
    // display operação [atualizar barra e tempo]
    // display erro

    // atualizar valores de status sempre

    switch (state) {
        case 0:
            ssd1306_draw_string(0, 0, "Bateria: ");
            char soc_str[10];
            snprintf(soc_str, sizeof(soc_str), "%d%%", battery_soc);
            ssd1306_draw_string(60, 0, soc_str);
            
            ssd1306_draw_string(0, 25, "Movimente o potenciometro");
            ssd1306_draw_string(0, 40, "para entrar na selecao");
            break;

        case 1:
            ssd1306_draw_string(0, 5, "Voltagem final");
            ssd1306_draw_string(0, 18, "selecionada:");
            
            char volt_str[16];
            snprintf(volt_str, sizeof(volt_str), "%.2f V", target_voltage);
            ssd1306_draw_string(20, 40, volt_str);
            break;

        case 2: {
            ssd1306_draw_string(0, 0, (char*)op_type);

            char prog_str[32];
            snprintf(prog_str, sizeof(prog_str), "Feito:%d%% Falta:%d%%", progress_percent, 100 - progress_percent);
            ssd1306_draw_string(0, 16, prog_str);

            int minutos = time_remaining_sec / 60;
            int segundos = time_remaining_sec % 60;
            char time_str[32];
            snprintf(time_str, sizeof(time_str), "Restante: %02d:%02d", minutos, segundos);
            ssd1306_draw_string(0, 32, time_str);

            char status_str[32];
            snprintf(status_str, sizeof(status_str), "%.1fV | %.2fA", current_v, current_i);
            ssd1306_draw_string(0, 48, status_str);
            break;
        }
    }

    char status_str[32];
    snprintf(status_str, sizeof(status_str), "%.1fV | %.2fA", current_v, current_i);
    ssd1306_draw_string(0, 112, status_str);

    ssd1306_show();
}


// ssd1306_draw_line(int x0, int y0, int x1, int y1, uint8_t color) ssd1306_draw_line(0, 15, 127, 15, 1)