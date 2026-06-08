#ifndef __SENSOR_DATA_H__
#define __SENSOR_DATA_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t  accel_x;
    int16_t  accel_y;
    int16_t  accel_z;
    uint32_t timestamp; 
} SensorData_t;

void SensorData_Init(void);
void SensorData_Update(int16_t x, int16_t y, int16_t z);
bool SensorData_Read(SensorData_t *out_data);

#endif