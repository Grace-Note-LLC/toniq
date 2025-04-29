#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include "bluetooth.h"
#include "tds.h"
#include "ultrasonic.h"

#define SPI_OP  SPI_OP_MODE_MASTER |SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE

static const struct spi_dt_spec display_dev =
                SPI_DT_SPEC_GET(DT_NODELABEL(my_spi_display), SPI_OP, 0);

// const struct gpio_dt_spec gpio_dev =
//                 GPIO_DT_SPEC_GET(DT_NODELABEL(cusgpio0), cusgpio_0);

#define GPIO_NODE DT_ALIAS(mycusgpio)

static const struct gpio_dt_spec gpio = GPIO_DT_SPEC_GET(GPIO_NODE, gpios);

// #define GPIO011_NODE   DT_ALIAS(mycusgpio)
// #define GPIO011  DT_GPIO_LABEL(GPIO011_NODE, gpios)
// const struct device *dev_gpio0;

int initialize(void) {
	int err;

    if (!gpio_is_ready_dt(&gpio)) {
		return -1;
	}

    if (!spi_is_ready_dt(&display_dev)) {
        return -1;
    }

	err = gpio_pin_configure_dt(&gpio, GPIO_OUTPUT_ACTIVE);
	if (err < 0) {
		return -1;
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
	if ((err = initialize())) {
		printk("Initialization failed (err %d)\n", err);
	}

    // uint8_t my_buffer[4] = {0};
    // struct spi_buf my_spi_buffer[1];
    // my_spi_buffer[0].buf = my_buffer;
    // my_spi_buffer[0].len = 4;
    // const struct spi_buf_set rx_buff = { my_spi_buffer, 1 };

    uint8_t my_buffer[4] = {0};
    struct spi_buf my_spi_buffer[1];
    my_spi_buffer[0].buf = my_buffer;
    my_spi_buffer[0].len = 4;
    const struct spi_buf_set tx_buff = { my_spi_buffer, 1 };


	while (true) {
		// Read sensors
		// int water_level = read_ultrasonic();
		// printk("Water level is %d\n", water_level);
		// int32_t quality_level = read_tds();
		// printk("Quality level is %d\n", quality_level);

        err = spi_write_dt(&display_dev, &tx_buff);
        if (err) { 
            printf("spi_write status: %d\n", err); 
        } else {
            printf("spi_write worked!\n"); 
        }

        err = gpio_pin_toggle_dt(&gpio);
		if (err < 0) {
            printf("failed to toggle gpio :(\n");
		}


		k_msleep(1000);
	}

	return 0;
}
