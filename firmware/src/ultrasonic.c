#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys_clock.h>
#include "ultrasonic.h"

#define GPIO_PORT        "GPIO_0"
#define TRIG_PIN         29
#define ECHO_PIN         30
#define TIMEOUT_MS       25U  /* 25 ms max echo wait */

static const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static struct gpio_callback gpio_cb;

volatile static uint32_t last_sample_rise_time = 0;
static float distance_cm = -1.0F;

static void gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    static bool high = false;
    uint32_t now = k_cycle_get_32();

    if (!high) {
        last_sample_rise_time = now;
        high = true;
    } else {
        uint32_t fall_time = now;
        uint32_t delta_cycles = fall_time - last_sample_rise_time;
        uint32_t duration_us = k_cyc_to_us_near32(delta_cycles);
        // printk("Pulse width: %u us\n", duration_us);
        // Use centimeters per microsecond conversion; divide by 2 because sound travels out and back
        distance_cm = ((float)duration_us * 0.0343F) / 2.0F;
        printf("Distance: %.2f cm\n", (double) distance_cm);
        high = false;
    }
}

static void trigger_pulse(void) {
    // Set TRIG high for 10us
    gpio_pin_set_raw(gpio0, TRIG_PIN, 1);
    k_usleep(10);
    gpio_pin_set_raw(gpio0, TRIG_PIN, 0);
}

/* Initialize GPIO for HC‑SR04 (no devicetree) */
int init_ultrasonic(void)
{
    int ret = 0;
    gpio_pin_configure(gpio0, TRIG_PIN, GPIO_OUTPUT);
    gpio_pin_configure(gpio0, ECHO_PIN, GPIO_INPUT);

    // Configure interrupt to measure pulse width on echo pin
    gpio_init_callback(&gpio_cb, gpio_callback, BIT(ECHO_PIN));
    gpio_add_callback(gpio0, &gpio_cb);
    gpio_pin_interrupt_configure(gpio0, ECHO_PIN, GPIO_INT_EDGE_BOTH);
    
    gpio_pin_set_raw(gpio0, TRIG_PIN, 0);
    return ret;
}

/* Trigger the sensor and return distance in cm, or -1 for timeout */
int read_ultrasonic_syncronous(void)
{
    uint32_t prev_sample_time = last_sample_rise_time;
    uint32_t num_sleeps = 0;

    trigger_pulse();
    while (last_sample_rise_time == prev_sample_time) {
        num_sleeps++;
        k_sleep(K_MSEC(TIMEOUT_MS / 2));
        if (num_sleeps >= 2) {
            return -1; // Timeout
        }
    }

    return distance_cm;
}