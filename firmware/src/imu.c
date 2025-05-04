#include "tds.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>


bool init_imu(void) {
    // Set up IMU and register interrupt when bottle flipped over

    return true;
}

int32_t read_imu(void) {
    return -1;
}