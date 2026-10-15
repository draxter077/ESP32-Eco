#pragma once

void display_init(void);
void display_update(int state, int battery_soc, float target_voltage, const char* op_type, int progress_percent, int time_remaining_sec, float current_v, float current_i);