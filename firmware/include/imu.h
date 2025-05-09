#ifndef IMU_H
#define IMU_H
#include <stdint.h>
#include <stdbool.h>

typedef struct gyro_data {
    bool valid;
    int16_t x_deg_s;
    int16_t y_deg_s;
    int16_t z_deg_s;
} gyro_data_t;

int init_imu(void);
gyro_data_t read_imu(void);

#endif // IMU_H