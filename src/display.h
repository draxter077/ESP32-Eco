void displayInit(void);
void displayDrawString(int x, int y, const char *str);
void displayRefresh(void);
void displayInitial(void);
void displayBatterySelect(int raw);
void displayFinalVoltageSelect(int raw, int batterySelected, float nowVoltage);
void displayInOperation(int isCharging, int isDischarging, float initialVoltage, float finalVoltage, float nowVoltage, float nowCurrent, int batterySelected);
void displayPreOperation(void);
void displayEmergency(void);