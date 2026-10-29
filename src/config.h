typedef struct {
    char name[20];
    float maxVoltage;
    float minVoltage;
} batteryData;

extern batteryData batteries[];

#define V_OFFSET 1