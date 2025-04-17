#include <zephyr/kernel.h>

#include "bluetooth.h"
#include "ultrasonic.h"

int initialize(void) {
	int err;

	/* Initialize the Bluetooth Subsystem */
	printk("Initialize Bluetooth\n");
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
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
		k_msleep(10000);
	}

	return 0;
}
