#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include "bluetooth.h"
#include "tds.h"
#include "ultrasonic.h"

#define SPI1_NODE DT_NODELABEL(spi1)
static const struct device* spi1_dev = DEVICE_DT_GET(SPI1_NODE);

#define MY_GPIO1 DT_NODELABEL(gpio1)
#define GPIO_1_CS 7

const struct device* gpio1_dev = DEVICE_DT_GET(MY_GPIO1);
 
static struct spi_config spi_cfg = {
	.frequency = 125000U,
	.operation = SPI_WORD_SET(8),
	.slave = 0,
};

int initialize(void) {
	int err;

    gpio_pin_configure(gpio1_dev, GPIO_1_CS, GPIO_OUTPUT);
	gpio_pin_set(gpio1_dev, GPIO_1_CS, 1);
    if (!device_is_ready(spi1_dev)) {
		printf("spi1_dev not ready\n");
		return 1;
    }

	/* Initialize the Bluetooth Subsystem */
	printk("Initialize Bluetooth\n");
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
        return -1;
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
    err = initialize();
	if (err) {
		printk("Initialization failed (err %d)\n", err);
	}

    // uint8_t my_buffer[4] = {0};
    // struct spi_buf my_spi_buffer[1];
    // my_spi_buffer[0].buf = my_buffer;
    // my_spi_buffer[0].len = 4;
    // const struct spi_buf_set rx_buff = { my_spi_buffer, 1 };

    // uint8_t my_buffer[4] = {0};
    // struct spi_buf my_spi_buffer[1];
    // my_spi_buffer[0].buf = my_buffer;
    // my_spi_buffer[0].len = 4;
    // const struct spi_buf_set tx_buff = { my_spi_buffer, 1 };

    uint8_t reg = 12;
    uint8_t value = 0x69;
    uint8_t tx_values[] = {(reg & 0x7F), value};

    const struct spi_buf tx_buf = {.buf = tx_values, .len = sizeof(tx_values)};
    const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	while (true) {
		// Read sensors
		// int water_level = read_ultrasonic();
		// printk("Water level is %d\n", water_level);
		// int32_t quality_level = read_tds();
		// printk("Quality level is %d\n", quality_level);

        gpio_pin_set(gpio1_dev, GPIO_1_CS, 0);
        err = spi_write(spi1_dev, &spi_cfg, &tx);
        gpio_pin_set(gpio1_dev, GPIO_1_CS, 1);

        if (err) { 
            printf("spi_write status: %d\n", err); 
        } else {
            printf("spi_write worked!\n"); 
        }
        // k_busy_wait(T_SCLK_NCS_WR);

  //       err = gpio_pin_toggle_dt(&gpio);
		// if (err < 0) {
  //           printf("failed to toggle gpio :(\n");
		// }


		k_msleep(1000);
	}

	return 0;
}
