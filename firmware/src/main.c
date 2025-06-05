#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "bluetooth.h"
#include "imu.h"
#include "tds.h"
#include "ultrasonic.h"
#include "zephyr/sys/printk.h"

LOG_MODULE_REGISTER(logging_main);
#define SAMPLE_BUFFER_COUNT 100

#define IMU
// #define TDS
// #define ULTRASONIC

typedef struct sampled_data {
	// Simultaneously counts number of samples stored and the zero-index of next sample
	int sample_count;
	int tds_level[SAMPLE_BUFFER_COUNT];
	int water_level[SAMPLE_BUFFER_COUNT];
} sampled_data_t;

static sampled_data_t data = {
	.sample_count = 0,
	.tds_level = {0},
	.water_level = {0},
};

int initialize(void) {
	int err;

	LOG_INF("Initialize Bluetooth\n");
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

#ifdef ULTRASONIC
	LOG_INF("Initialize Ultrasonic sensor\n");
	err = init_ultrasonic();
	if (err) {
		printk("Ultrasonic init failed (err %d)\n", err);
		return err;
	}
#endif

#ifdef IMU
	LOG_INF("Initialize IMU\n");
	err = init_imu();
	if (err) {
		printk("IMU init failed (err %d)\n", err);
		return err;
	}
#endif

#ifdef TDS
	LOG_INF("Initialize TDS\n");
	err = init_tds();
	if (err) {
		printk("TDS init failed (err %d)\n", err);
		return err;
	}
#endif

	return 0;
}

// Main loop triggers samples upon timeout and IMU tilt interrupt (not implemented yet)
int take_sample() {
	int err, tds_reading;
#ifdef IMU
    gyro_data_t raw_gyro = read_imu();
    if (!raw_gyro.valid) {
        LOG_ERR("IMU read failed");
        return err;
    }
    // LOG_INF("gyro_x: %d deg/s", raw_gyro.x_deg_s);
    // LOG_INF("gyro_y: %d deg/s", raw_gyro.y_deg_s);
    // LOG_INF("gyro_z: %d deg/s", raw_gyro.z_deg_s);
	return raw_gyro.z_deg_s;

#endif

#ifdef TDS
	// Read TDS
	err = tds_reading = read_tds();
	if (tds_reading < 0) {
		printk("TDS read failed (err %d)\n", err);
		return err;
	}
	LOG_INF("TDS value (ppm): %d\n", tds_reading);
#endif

#ifdef ULTRASONIC
	// Read Ultrasonic
	int water_level = read_ultrasonic_syncronous();
	if (water_level < 0) {
		printk("Ultrasonic read failed (err %d)\n", err);
		return err;
	}
	LOG_INF("Water level is %d cm\n", water_level);
#endif

#ifdef IMU
#ifdef ULTRASONIC
	// Store the readings in the data structure
	int sample_count = data.sample_count;
	if (sample_count < SAMPLE_BUFFER_COUNT) {
		data.tds_level[sample_count] = tds_reading;
		data.water_level[sample_count] = water_level;
		data.sample_count++;
	} else {
		printk("Sample buffer full, cannot store more samples\n");
		return -50;
	}
#endif
#endif

	return 0;
}

int main(void) {
	int err;
	if ((err = initialize())) {
		printk("Initialization failed (err %d)\n", err);
	}
	init_tds();


	while (true) {
		int gforce_z = take_sample();
		if (gforce_z <= -10000) {
			int tds_val = read_tds();
			printk("tds_val: %d", tds_val);
		}

		// LOG_INF("tds_val: %d", tds_val);

		k_sleep(K_MSEC(100));
	}

	return 0;
}
