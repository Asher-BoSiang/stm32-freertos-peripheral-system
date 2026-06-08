#include "sensor_data.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h" 

static SensorData_t      s_sensor_data = {0}; 
static SemaphoreHandle_t s_sensor_mutex = NULL;

void SensorData_Init(void) {
    s_sensor_mutex = xSemaphoreCreateMutex();
}

void SensorData_Update(int16_t x, int16_t y, int16_t z) {
    if (xSemaphoreTake(s_sensor_mutex, portMAX_DELAY) == pdTRUE) {
        s_sensor_data.accel_x = x;
        s_sensor_data.accel_y = y;
        s_sensor_data.accel_z = z;
        s_sensor_data.timestamp = xTaskGetTickCount();
        xSemaphoreGive(s_sensor_mutex);
    }
}

bool SensorData_Read(SensorData_t *out_data) {
    if (xSemaphoreTake(s_sensor_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        *out_data = s_sensor_data;
        xSemaphoreGive(s_sensor_mutex);
        return true;
    }
    return false;
}