#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include "bluetooth.h"
#include "tds.h"
#include "ultrasonic.h"

int initialize(void) {
	int err;

	/* Initialize the Bluetooth Subsystem */
	printk("Initialize Bluetooth\n");
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	/* Initialize the TDS Sensor */
	printk("Initialize TDS\n");
	if (!init_tds()) {
		printk("TDS init failed\n");
		return -1;
	}

	return err;
}

int main(void) {
	int err;
	if ((err = initialize())) {
		printk("Initialization failed (err %d)\n", err);
	}

	while (true) {
		// Read sensors
		int water_level = read_ultrasonic();
		printk("Water level is %d\n", water_level);
		int32_t quality_level = read_tds();
		printk("Quality level is %d\n", quality_level);
		k_msleep(1000);
	}

	return 0;
}
