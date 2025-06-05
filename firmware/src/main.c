#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include "eink/epd_driver.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

// Device tree nodes for SPI and GPIO
#define SPI1_NODE DT_NODELABEL(spi1) // this defines SCK: 31, MOSI/DIN: 30
static const struct device* spi1_dev = DEVICE_DT_GET(SPI1_NODE);
const struct device* gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
const struct device* gpio1_dev = DEVICE_DT_GET(DT_NODELABEL(gpio1));

// Increase stack size for display operations
#define STACK_SIZE 4096
#define PRIORITY 5

// Pin definitions
#define EPD_CS_PIN 7    // GPIO1_7
#define EPD_DC_PIN 21   // GPIO0_21
#define EPD_RST_PIN 24  // GPIO0_24
#define EPD_BUSY_PIN 22 // GPIO0_22

// Display dimensions (200x200 for D variant)
#define EPD_2IN13D_WIDTH (EPD_WIDTH)
#define EPD_2IN13D_HEIGHT (EPD_HEIGHT)

// Display instance
static epd_driver_t display;

// Static buffer for display data
static uint8_t display_buffer[EPD_BUFFER_SIZE];

// Stack for main thread
K_THREAD_STACK_DEFINE(main_stack, STACK_SIZE);

int main(void) {
    // Check devices are ready
    if (!device_is_ready(spi1_dev) || !device_is_ready(gpio0_dev) || !device_is_ready(gpio1_dev)) {
        LOG_ERR("Devices not ready");
        return 0;
    }

    LOG_INF("Devices ready - GPIO0: %p, GPIO1: %p, SPI: %p", gpio0_dev, gpio1_dev, spi1_dev);
    k_msleep(100);  // Longer delay to ensure stable power

    // Configure GPIO pins
    LOG_INF("Starting GPIO configuration...");
    k_msleep(10);

    // Configure and verify DC pin
    LOG_INF("Configuring DC pin (GPIO0_%d)", EPD_DC_PIN);
    int ret = gpio_pin_configure(gpio0_dev, EPD_DC_PIN, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_ERR("Failed to configure DC pin: %d", ret);
        return 0;
    }
    k_msleep(5);
    int val = gpio_pin_get(gpio0_dev, EPD_DC_PIN);
    LOG_INF("DC pin state after config: %d", val);
    ret = gpio_pin_set(gpio0_dev, EPD_DC_PIN, 1);
    if (ret != 0) {
        LOG_ERR("Failed to set DC pin high: %d", ret);
        return 0;
    }
    k_msleep(5);
    val = gpio_pin_get(gpio0_dev, EPD_DC_PIN);
    LOG_INF("DC pin state after set high: %d", val);
    k_msleep(10);

    // Configure and verify BUSY pin
    LOG_INF("Configuring BUSY pin (GPIO0_%d)", EPD_BUSY_PIN);
    ret = gpio_pin_configure(gpio0_dev, EPD_BUSY_PIN, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Failed to configure BUSY pin: %d", ret);
        return 0;
    }
    k_msleep(5);
    val = gpio_pin_get(gpio0_dev, EPD_BUSY_PIN);
    LOG_INF("BUSY pin state: %d", val);
    k_msleep(10);

    // Configure and verify RST pin
    LOG_INF("Configuring RST pin (GPIO0_%d)", EPD_RST_PIN);
    ret = gpio_pin_configure(gpio0_dev, EPD_RST_PIN, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_ERR("Failed to configure RST pin: %d", ret);
        return 0;
    }
    k_msleep(5);
    val = gpio_pin_get(gpio0_dev, EPD_RST_PIN);
    LOG_INF("RST pin state after config: %d", val);
    ret = gpio_pin_set(gpio0_dev, EPD_RST_PIN, 1);
    if (ret != 0) {
        LOG_ERR("Failed to set RST pin high: %d", ret);
        return 0;
    }
    k_msleep(5);
    val = gpio_pin_get(gpio0_dev, EPD_RST_PIN);
    LOG_INF("RST pin state after set high: %d", val);
    k_msleep(10);

    // Configure and verify CS pin
    LOG_INF("Configuring CS pin (GPIO1_%d)", EPD_CS_PIN);
    ret = gpio_pin_configure(gpio1_dev, EPD_CS_PIN, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_ERR("Failed to configure CS pin: %d", ret);
        return 0;
    }
    k_msleep(5);
    val = gpio_pin_get(gpio1_dev, EPD_CS_PIN);
    LOG_INF("CS pin state after config: %d", val);
    ret = gpio_pin_set(gpio1_dev, EPD_CS_PIN, 1);
    if (ret != 0) {
        LOG_ERR("Failed to set CS pin high: %d", ret);
        return 0;
    }
    k_msleep(5);
    val = gpio_pin_get(gpio1_dev, EPD_CS_PIN);
    LOG_INF("CS pin state after set high: %d", val);
    k_msleep(10);

    LOG_INF("GPIO configuration complete");
    k_msleep(100);  // Longer delay after GPIO config

    // Store device pointers
    LOG_INF("Storing device pointers");
    display.spi_dev = spi1_dev;
    display.gpio0_dev = gpio0_dev;
    display.gpio1_dev = gpio1_dev;
    LOG_INF("Device pointers stored");
    k_msleep(100);  // Longer delay after storing pointers

    // Initialize display
    LOG_INF("Initializing display");
    if (epd_init(&display, EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN, gpio0_dev, gpio1_dev, spi1_dev) != 0) {
        LOG_ERR("Display init failed");
        return 0;
    }
    LOG_INF("Display initialized");
    k_msleep(100);  // Longer delay after init
    
    // Clear screen to white
    LOG_INF("Clearing screen to white");
    epd_clear_screen(&display, 0xFF);
    LOG_INF("Screen cleared");
    k_msleep(100);  // Longer delay after clear

    // Initialize display buffer (all white)
    LOG_INF("Initializing display buffer");
    memset(display_buffer, 0xFF, EPD_BUFFER_SIZE);
    LOG_INF("Display buffer initialized");
    k_msleep(100);  // Longer delay after buffer init

    // Draw a simple pattern in the middle
    LOG_INF("Drawing test pattern");
    for (int i = 0; i < 20; i++) {
        display_buffer[100 * (EPD_WIDTH / 8) + i] = 0x55;  // Draw a black line
    }
    LOG_INF("Pattern drawn");
    k_msleep(100);  // Longer delay after drawing
    
    // Write to display and refresh
    LOG_INF("Writing pattern to display");
    epd_write_image(&display, display_buffer, 0, 0, EPD_WIDTH, EPD_HEIGHT, false, false);
    LOG_INF("Pattern written to display");
    k_msleep(100);  // Longer delay after write

    LOG_INF("Refreshing display");
    epd_refresh(&display, false);
    LOG_INF("Display refreshed");
    k_msleep(2000);  // Wait 2 seconds to see the update

    // Power off
    LOG_INF("Powering off display");
    display.power_off(&display);
    LOG_INF("Display powered off");
    
    // Done
    while (1) {
        k_msleep(1000);
        LOG_INF("Still alive...");
    }

    return 0;
}
