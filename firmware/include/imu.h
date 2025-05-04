#ifndef IMU_H
#define IMU_H
#include <stdint.h>
#include <stdbool.h>

bool init_imu(void);
int32_t read_imu(void);

#endif // IMU_H