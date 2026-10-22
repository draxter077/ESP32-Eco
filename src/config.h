typedef struct {
    char name[20];
    float maxVoltage;
    float minVoltage;
} batteryData;

batteryData batteries[5] = {
    [0] = {"Bateria Teste1", 42.0f, 3.0f},
    [1] = {"Bateria Teste2", 42.0f, 3.0f},
    [2] = {"Bateria Teste3", 42.0f, 3.0f},
    [3] = {"Bateria Teste4", 42.0f, 3.0f},
    [4] = {"Bateria Teste5", 42.0f, 3.0f},
};