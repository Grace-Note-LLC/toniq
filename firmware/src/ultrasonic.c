#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <zephyr/drivers/adc.h>

#define TRIG_PIN 29
#define ECHO_PIN 30

const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

volatile static uint32_t rise_time = 0;

static void gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    static bool high = false;
    uint32_t now = k_cycle_get_32();

    if (!high) {
        rise_time = now;
        high = true;
    } else {
        uint32_t fall_time = now;
        uint32_t delta_cycles = fall_time - rise_time;
        uint32_t duration_us = k_cyc_to_us_near32(delta_cycles);
        // printk("Pulse width: %u us\n", duration_us);
        // Use centimeters per microsecond conversion; divide by 2 because sound travels out and back
        float distance_cm = ((float)duration_us * 0.0343) / 2;
        printf("Distance: %.2f cm\n", distance_cm);
        high = false;
    }
}

static struct gpio_callback gpio_cb;

int main(void) {
    printf("STARTING...\n");

    gpio_pin_configure(gpio0, TRIG_PIN, GPIO_OUTPUT);
    gpio_pin_configure(gpio0, ECHO_PIN, GPIO_INPUT);

    // Configure interrupt to measure pulse width on echo pin
    gpio_init_callback(&gpio_cb, gpio_callback, BIT(ECHO_PIN));
    gpio_add_callback(gpio0, &gpio_cb);
    gpio_pin_interrupt_configure(gpio0, ECHO_PIN, GPIO_INT_EDGE_BOTH);

    gpio_pin_set_raw(gpio0, TRIG_PIN, 0);

    while (1) {
        // Set TRIG high for 10us
        gpio_pin_set_raw(gpio0, TRIG_PIN, 1);
        k_usleep(10);

        gpio_pin_set_raw(gpio0, TRIG_PIN, 0);

        k_msleep(100);
    }
}