#pragma once

#include "config.h"
#include <stdbool.h>

void hardware_init(void);
void read_voltage(void);
void read_current(void);
void read_temperature(void);
void read_potenciometer(void);
void calibrate_current_zero(void);
void emergency_valve(void);