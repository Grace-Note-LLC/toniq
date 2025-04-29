#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

// #include "bluetooth.h"
// #include "tds.h"
// #include "ultrasonic.h"

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

static void setRegister(uint8_t reg, uint8_t value) {
	int err;

	uint8_t tx_values[] = {(reg & 0x7F), value};

	struct spi_buf tx_spi_bufs[] = {
		{ .buf = tx_values, .len = sizeof(tx_values) }
	};

	struct spi_buf_set spi_tx_buffer_set = {
		.buffers = tx_spi_bufs,
		.count = 1
	};

	gpio_pin_set(gpio1_dev, GPIO_1_CS, 0);
	err = spi_write(spi1_dev, &spi_cfg, &spi_tx_buffer_set);
	gpio_pin_set(gpio1_dev, GPIO_1_CS, 1);

	if (err < 0) {
		printk("setRegister failed: %d\n", err);
	}
}

int main(void) {

	gpio_pin_configure(gpio1_dev, GPIO_1_CS, GPIO_OUTPUT);
	gpio_pin_set(gpio1_dev, GPIO_1_CS, 1);

	if (!device_is_ready(spi1_dev)) {
		printk("spi1_dev not ready\n");
		return 1;
	}

	while (true) {
		setRegister(0x69, 0x69);
		k_msleep(1000);
	}

	return 0;
}
