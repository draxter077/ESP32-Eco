#include <stdio.h>
#include <stdbool.h>
#include "esp_spiffs.h"
#include <sys/unistd.h>
#include <sys/stat.h>

#define LOG_FILE_PATH "/storage/logs.bin"
#define BATCH_SIZE 60
#define MAX_RECORDS 90000

static int totalRecords = 0;

typedef struct __attribute__((packed)) {
    int timestamp;
    float current;
    float voltage;
} log_record_t;

static log_record_t ram_buffer[BATCH_SIZE];
static int buffer_index = 0;

void initStorage(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/storage",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_vfs_spiffs_register(&conf);
}


void flushBufferToFlash(void) {
    if (buffer_index <= 0) return;
    FILE *f = fopen(LOG_FILE_PATH, "ab");
    if(!f){
        printf("Erro ao abrir arquivo\n");
        exit(1);
    }
    fwrite(ram_buffer, sizeof(log_record_t), buffer_index, f);
    fclose(f);
    buffer_index = 0;
}

void saveLogRecord(int nowTime, float nowCurrent, float nowVoltage){

     if (totalRecords >= MAX_RECORDS) {
        return;
    }


    ram_buffer[buffer_index].timestamp = (int) nowTime/(1000*1000);
    ram_buffer[buffer_index].current = nowCurrent;
    ram_buffer[buffer_index].voltage = nowVoltage;
    buffer_index++;
    totalRecords++;

    if (buffer_index >= BATCH_SIZE) {
        flushBufferToFlash();
    }
}