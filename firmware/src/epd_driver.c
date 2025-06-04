#include "eink/epd_driver.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/mem_manage.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(epd_driver, LOG_LEVEL_DBG);

// Static buffer for display data
static uint8_t display_buffer[EPD_BUFFER_SIZE];

// Static function declarations
static void epd_write_command_impl(epd_driver_t* driver, uint8_t command);
static void epd_write_data_impl(epd_driver_t* driver, uint8_t data);
static void epd_wait_while_busy_impl(epd_driver_t* driver, const char* msg, uint16_t timeout);
static void epd_reset_impl(epd_driver_t* driver);
static void epd_refresh_impl(epd_driver_t* driver, bool partial_update_mode);
static void epd_power_off_impl(epd_driver_t* driver);
static void epd_hibernate_impl(epd_driver_t* driver);
static void epd_set_partial_ram_area_impl(epd_driver_t* driver, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

// // Helper function for busy waiting
// static void busy_wait_ms(uint32_t ms) {
//     // Assuming 64MHz system clock, each cycle is ~15.6ns
//     // So 1ms is roughly 64,000 cycles
//     const uint32_t cycles_per_ms = 64000;
//     for (uint32_t i = 0; i < ms * cycles_per_ms; i++) {
//         __asm__ __volatile__("nop");
//     }
// }

// Initialize the display driver
int epd_init(epd_driver_t* driver, int16_t cs_pin, int16_t dc_pin, int16_t rst_pin, int16_t busy_pin,
             const struct device* gpio0_dev, const struct device* gpio1_dev, const struct device* spi_dev) {
    if (!driver) {
        LOG_ERR("Driver pointer is NULL");
        return -1;
    }

    LOG_INF("Starting display initialization for 1.54\" display");
    
    // Initialize driver structure
    memset(driver, 0, sizeof(epd_driver_t));
    LOG_DBG("Driver structure cleared");
    
    // Set pin configuration
    driver->cs_pin = cs_pin;
    driver->dc_pin = dc_pin;
    driver->rst_pin = rst_pin;
    driver->busy_pin = busy_pin;
    LOG_DBG("Pin configuration set - CS: %d, DC: %d, RST: %d, BUSY: %d", 
            cs_pin, dc_pin, rst_pin, busy_pin);
    
    // Store device pointers
    driver->gpio0_dev = gpio0_dev;
    driver->gpio1_dev = gpio1_dev;
    driver->spi_dev = spi_dev;
    LOG_DBG("Device pointers stored - GPIO0: %p, GPIO1: %p, SPI: %p", 
            (void*)driver->gpio0_dev, (void*)driver->gpio1_dev, (void*)driver->spi_dev);
    
    // Initialize SPI configuration
    driver->spi_config.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | 
                                 SPI_OP_MODE_MASTER;  // Mode 0 (CPOL=0, CPHA=0)
    driver->spi_config.frequency = 4000000; // 4MHz
    driver->spi_config.slave = 0;
    LOG_DBG("SPI configuration set - Mode: 0, Freq: 4MHz");
    
    // Set function pointers
    driver->write_command = epd_write_command_impl;
    driver->write_data = epd_write_data_impl;
    driver->wait_while_busy = epd_wait_while_busy_impl;
    driver->reset = epd_reset_impl;
    driver->refresh = epd_refresh_impl;
    driver->power_off = epd_power_off_impl;
    driver->hibernate = epd_hibernate_impl;
    LOG_DBG("Function pointers set");
    
    // Set timing parameters for 1.54" display
    driver->power_on_time = 200;  // Increased for 1.54"
    driver->power_off_time = 200; // Increased for 1.54"
    driver->full_refresh_time = 3000; // Increased for 1.54"
    driver->partial_refresh_time = 1000; // Increased for 1.54"
    LOG_DBG("Timing parameters set for 1.54\" display");
    
    // Use static buffer
    driver->buffer = display_buffer;
    driver->buffer_size = EPD_BUFFER_SIZE;
    LOG_DBG("Buffer configured - Size: %d bytes", EPD_BUFFER_SIZE);
    
    // Initialize display
    LOG_INF("Resetting display");
    driver->reset(driver);
    LOG_DBG("Reset complete, waiting 20ms");
    k_msleep(20);
    
    // Send initialization commands for 1.54" display
    LOG_INF("Sending initialization commands");
    
    // Software reset
    LOG_DBG("Sending software reset command");
    driver->write_command(driver, 0x12);
    driver->wait_while_busy(driver, "software reset", 20);
    LOG_DBG("Software reset complete");
        
    LOG_DBG("Sending Driver Output Control (0x01)");
    driver->write_command(driver, 0x01);
    driver->write_data(driver, 0xC7); // (200-1) = 199 decimal
    driver->write_data(driver, 0x00);
    driver->write_data(driver, 0x00); // Gate Scan Start Position LSB 00, MSB 00 = 0

    LOG_DBG("Sending Border Waveform Control (0x3C)");
    driver->write_command(driver, 0x3C);
    driver->write_data(driver, 0x05); // Refer to GDEH0154D67 datasheet or GxEPD2 for exact value meaning

    // This command (0x18 Temperature Sensor) is often optional for basic display operation but good for consistency.
    // LOG_DBG("Sending Temperature Sensor Control (0x18)");
    // driver->write_command(driver, 0x18);
    // driver->write_data(driver, 0x80); // Enable internal sensor

    // The original epd_driver.c has many other commands (Power setting, Booster, PLL, VCM, VCOM)
    // These might be necessary or might conflict. Start with the GxEPD2 minimal init.
    // If issues persist, consult the GDEH0154D67 datasheet to see if commands like
    // Power Setting (0x01 with different params), Booster (0x06), Power On (0x04)
    // are needed *before* or *interspersed* with the GxEPD2 sequence.
    // GxEPD2 typically handles power on/off via separate _PowerOn() / _PowerOff() methods which are
    // called by refresh routines. Your epd_driver.c has 0x04 (POWER_ON).
    // For now, let's assume the GxEPD2 sequence above is more targeted.
    // Ensure Power On (0x04) is called if not part of the refresh sequence logic.
    // The GxEPD2 lib calls _PowerOn in its refresh/display methods.
    // Your current epd_driver.c calls POWER_ON (0x04) and waits. This is probably good.
    // It should be: SW_RESET -> Driver Output -> Border Waveform -> (Optional Temp) -> POWER_ON

    LOG_DBG("Sending power on command (0x04)");
    driver->write_command(driver, 0x04);
    driver->wait_while_busy(driver, "power on", driver->power_on_time); // power_on_time might need adjustment
    LOG_DBG("Power on complete");

    // RAM Address and Data Entry Mode settings:
    // GxEPD2 calls _setPartialRamArea, which includes Data Entry Mode (0x11).
    // It's good to set the data entry mode and initial RAM window.
    LOG_DBG("Setting Data Entry Mode (0x11)");
    driver->write_command(driver, 0x11);
    driver->write_data(driver, 0x03); // X increment, Y increment

    LOG_DBG("Setting RAM X/Y Area and Pointers");
    // Call epd_set_partial_ram_area_impl to set full screen initially
    epd_set_partial_ram_area_impl(driver, 0, 0, EPD_WIDTH, EPD_HEIGHT);
        
    driver->init_display_done = true;
    driver->power_is_on = true;
    LOG_INF("Display initialization complete for 1.54\" display");
    return 0;
}

// Deinitialize the display driver
void epd_deinit(epd_driver_t* driver) {
    if (!driver) return;
    
    // No need to free static buffer
    driver->buffer = NULL;
    driver->buffer_size = 0;
    
    driver->power_off(driver);
    driver->hibernate(driver);
}

// Write a command to the display
static void epd_write_command_impl(epd_driver_t* driver, uint8_t command) {
    if (!driver || !driver->spi_dev || !driver->gpio0_dev || !driver->gpio1_dev) {
        LOG_ERR("Invalid driver state for command write - driver: %p, spi: %p, gpio0: %p, gpio1: %p",
                (void*)driver, 
                (void*)(driver ? driver->spi_dev : NULL),
                (void*)(driver ? driver->gpio0_dev : NULL),
                (void*)(driver ? driver->gpio1_dev : NULL));
        return;
    }
    
    LOG_DBG("Writing command: 0x%02x", command);
    
    // Set DC low for command
    int ret = gpio_pin_set(driver->gpio0_dev, driver->dc_pin, 0);
    if (ret != 0) {
        LOG_ERR("Failed to set DC pin: %d", ret);
        return;
    }
    LOG_DBG("DC pin set low for command");
    
    // Set CS low to start transaction
    ret = gpio_pin_set(driver->gpio1_dev, driver->cs_pin, 0);
    if (ret != 0) {
        LOG_ERR("Failed to set CS pin: %d", ret);
        return;
    }
    LOG_DBG("CS pin set low, starting SPI transaction");
    
    const struct spi_buf tx_buf = {
        .buf = &command,
        .len = 1
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };
    
    ret = spi_write(driver->spi_dev, &driver->spi_config, &tx);
    if (ret != 0) {
        LOG_ERR("SPI write failed: %d", ret);
    } else {
        LOG_DBG("SPI write successful");
    }
    
    // Set CS high to end transaction
    ret = gpio_pin_set(driver->gpio1_dev, driver->cs_pin, 1);
    if (ret != 0) {
        LOG_ERR("Failed to clear CS pin: %d", ret);
    } else {
        LOG_DBG("CS pin set high, transaction complete");
    }
    
    LOG_DBG("Command write complete");
}

// Write data to the display
static void epd_write_data_impl(epd_driver_t* driver, uint8_t data) {
    if (!driver || !driver->spi_dev || !driver->gpio0_dev || !driver->gpio1_dev) return;
    
    // Set DC high for data
    gpio_pin_set(driver->gpio0_dev, driver->dc_pin, 1);
    // Set CS low to start transaction
    gpio_pin_set(driver->gpio1_dev, driver->cs_pin, 0);
    
    const struct spi_buf tx_buf = {
        .buf = &data,
        .len = 1
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };
    
    spi_write(driver->spi_dev, &driver->spi_config, &tx);
    
    // Set CS high to end transaction
    gpio_pin_set(driver->gpio1_dev, driver->cs_pin, 1);
}

// Wait while display is busy
static void epd_wait_while_busy_impl(epd_driver_t* driver, const char* msg, uint16_t timeout) {
    if (!driver || !driver->gpio0_dev) return;
    
    uint32_t start_time = k_uptime_get_32();
    while (gpio_pin_get(driver->gpio0_dev, driver->busy_pin) == 1) {
        if (k_uptime_get_32() - start_time > timeout) {
            break;
        }
        k_msleep(10);
    }
}

// Reset the display
static void epd_reset_impl(epd_driver_t* driver) {
    if (!driver || !driver->gpio0_dev) {
        LOG_ERR("Invalid driver state for reset - driver: %p, gpio0: %p", 
                (void*)driver, (void*)(driver ? driver->gpio0_dev : NULL));
        return;
    }
    
    LOG_INF("Starting display reset sequence");
    
    // Ensure RST pin is configured as output
    int ret = gpio_pin_configure(driver->gpio0_dev, driver->rst_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_ERR("Failed to configure RST pin as output: %d", ret);
        return;
    }
    LOG_DBG("RST pin configured as output");
    
    // Initial state - RST high
    LOG_DBG("Setting RST pin high (initial state)");
    ret = gpio_pin_set(driver->gpio0_dev, driver->rst_pin, 1);
    if (ret != 0) {
        LOG_ERR("Failed to set RST pin high: %d", ret);
        return;
    }
    
    // Verify the pin state
    int val = gpio_pin_get(driver->gpio0_dev, driver->rst_pin);
    LOG_DBG("RST pin state after setting high: %d", val);
    if (val != 1) {
        LOG_ERR("RST pin not set high, got: %d", val);
        return;
    }
    
    // Wait for power to stabilize
    LOG_DBG("Waiting 100ms for power stabilization");
    k_msleep(100);
    
    // Reset pulse - RST low
    LOG_DBG("Setting RST pin low (reset pulse)");
    ret = gpio_pin_set(driver->gpio0_dev, driver->rst_pin, 0);
    if (ret != 0) {
        LOG_ERR("Failed to set RST pin low: %d", ret);
        return;
    }
    
    // Verify the pin state
    val = gpio_pin_get(driver->gpio0_dev, driver->rst_pin);
    LOG_DBG("RST pin state after setting low: %d", val);
    if (val != 0) {
        LOG_ERR("RST pin not set low, got: %d", val);
        return;
    }
    
    // Wait for reset pulse
    LOG_DBG("Waiting 20ms during reset pulse");
    k_msleep(20);
    
    // End reset - RST high
    LOG_DBG("Setting RST pin high (end reset)");
    ret = gpio_pin_set(driver->gpio0_dev, driver->rst_pin, 1);
    if (ret != 0) {
        LOG_ERR("Failed to set RST pin high: %d", ret);
        return;
    }
    
    // Verify the pin state
    val = gpio_pin_get(driver->gpio0_dev, driver->rst_pin);
    LOG_DBG("RST pin state after end reset: %d", val);
    if (val != 1) {
        LOG_ERR("RST pin not set high after reset, got: %d", val);
        return;
    }
    
    // Wait for display to stabilize after reset
    LOG_DBG("Waiting 200ms after reset for display stabilization");
    k_msleep(200);
    
    LOG_INF("Reset sequence complete");
}

// Set partial RAM area
static void epd_set_partial_ram_area_impl(epd_driver_t* driver, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    driver->write_command(driver, 0x11); // set ram entry mode
    driver->write_data(driver, 0x03);    // x increase, y increase : normal mode
    driver->write_command(driver, 0x44);
    driver->write_data(driver, x / 8);
    driver->write_data(driver, (x + w - 1) / 8);
    driver->write_command(driver, 0x45);
    driver->write_data(driver, y % 256);
    driver->write_data(driver, y / 256);
    driver->write_data(driver, (y + h - 1) % 256);
    driver->write_data(driver, (y + h - 1) / 256);
    driver->write_command(driver, 0x4e);
    driver->write_data(driver, x / 8);
    driver->write_command(driver, 0x4f);
    driver->write_data(driver, y % 256);
    driver->write_data(driver, y / 256);
}

// Clear the screen
void epd_clear_screen(epd_driver_t* driver, uint8_t value) {
    if (!driver || !driver->init_display_done) return;
    
    driver->write_command(driver, 0x24); // write RAM for black(0)/white (1)
    for (uint32_t i = 0; i < EPD_BUFFER_SIZE; i++) {
        driver->write_data(driver, value);
    }
    driver->refresh(driver, false);
}

// Write screen buffer
void epd_write_screen_buffer(epd_driver_t* driver, uint8_t value) {
    if (!driver || !driver->init_display_done) return;
    
    if (driver->initial_write) {
        epd_clear_screen(driver, value);
        return;
    }
    
    driver->write_command(driver, 0x24);
    for (uint32_t i = 0; i < EPD_BUFFER_SIZE; i++) {
        driver->write_data(driver, value);
    }
}

// Write image to display
void epd_write_image(epd_driver_t* driver, const uint8_t* bitmap, int16_t x, int16_t y, 
                    int16_t w, int16_t h, bool invert, bool mirror_y) {
    if (!driver || !driver->init_display_done || !bitmap) return;
    
    int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8; // byte boundary
    w = wb * 8; // byte boundary
    
    int16_t x1 = x < 0 ? 0 : x;
    int16_t y1 = y < 0 ? 0 : y;
    int16_t w1 = x + w < EPD_WIDTH ? w : EPD_WIDTH - x;
    int16_t h1 = y + h < EPD_HEIGHT ? h : EPD_HEIGHT - y;
    
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx;
    h1 -= dy;
    
    if ((w1 <= 0) || (h1 <= 0)) return;
    
    if (driver->initial_write) {
        epd_write_screen_buffer(driver, 0xFF);
    }
    
    epd_set_partial_ram_area_impl(driver, x1, y1, w1, h1);
    driver->write_command(driver, 0x24);
    
    for (int16_t i = 0; i < h1; i++) {
        for (int16_t j = 0; j < w1 / 8; j++) {
            uint8_t data;
            int16_t idx = mirror_y ? j + dx / 8 + ((h - 1 - (i + dy))) * wb : j + dx / 8 + (i + dy) * wb;
            data = bitmap[idx];
            if (invert) data = ~data;
            driver->write_data(driver, data);
        }
    }
}

// Refresh implementation
static void epd_refresh_impl(epd_driver_t* driver, bool partial_update_mode) {
    printk("epd_refresh: Starting refresh (partial: %d)\n", partial_update_mode);
    if (!driver || !driver->init_display_done) {
        printk("epd_refresh: Invalid driver state\n");
        return;
    }
    
    if (!partial_update_mode) {
        printk("epd_refresh: Sending refresh commands\n");
        driver->write_command(driver, EPD_CMD_DISPLAY_UPDATE_CONTROL);
        driver->write_data(driver, 0xf7);
        driver->write_command(driver, EPD_CMD_MASTER_ACTIVATION);
        driver->wait_while_busy(driver, "refresh", driver->full_refresh_time);
        printk("epd_refresh: Refresh complete\n");
    }
}

// Power off implementation
static void epd_power_off_impl(epd_driver_t* driver) {
    printk("epd_power_off: Starting power off sequence\n");
    if (!driver || !driver->init_display_done) {
        printk("epd_power_off: Invalid driver state\n");
        return;
    }
    
    if (driver->power_is_on) {
        printk("epd_power_off: Sending power off commands\n");
        driver->write_command(driver, EPD_CMD_DISPLAY_UPDATE_CONTROL);
        driver->write_data(driver, 0x83);
        driver->write_command(driver, EPD_CMD_MASTER_ACTIVATION);
        driver->wait_while_busy(driver, "power off", driver->power_off_time);
        driver->power_is_on = false;
        printk("epd_power_off: Power off complete\n");
    } else {
        printk("epd_power_off: Display already powered off\n");
    }
}

// Hibernate implementation
static void epd_hibernate_impl(epd_driver_t* driver) {
    if (!driver || !driver->init_display_done) return;
    
    if (!driver->hibernating) {
        driver->write_command(driver, EPD_CMD_DEEP_SLEEP);
        driver->write_data(driver, 0x01);
        driver->hibernating = true;
    }
}

// Refresh the display
void epd_refresh(epd_driver_t* driver, bool partial_update_mode) {
    if (!driver || !driver->init_display_done) return;
    
    if (partial_update_mode) {
        epd_refresh_area(driver, 0, 0, EPD_WIDTH, EPD_HEIGHT);
    } else {
        driver->write_command(driver, EPD_CMD_DISPLAY_UPDATE_CONTROL);
        driver->write_data(driver, 0xf7);
        driver->write_command(driver, EPD_CMD_MASTER_ACTIVATION);
        driver->wait_while_busy(driver, "refresh", driver->full_refresh_time);
        driver->power_is_on = false;
    }
}

// Refresh a specific area
void epd_refresh_area(epd_driver_t* driver, int16_t x, int16_t y, int16_t w, int16_t h) {
    if (!driver || !driver->init_display_done) return;
    
    if (driver->initial_refresh) {
        epd_refresh(driver, false);
        return;
    }
    
    int16_t w1 = x < 0 ? w + x : w;
    int16_t h1 = y < 0 ? h + y : h;
    int16_t x1 = x < 0 ? 0 : x;
    int16_t y1 = y < 0 ? 0 : y;
    
    w1 = x1 + w1 < EPD_WIDTH ? w1 : EPD_WIDTH - x1;
    h1 = y1 + h1 < EPD_HEIGHT ? h1 : EPD_HEIGHT - y1;
    
    if ((w1 <= 0) || (h1 <= 0)) return;
    
    w1 += x1 % 8;
    if (w1 % 8 > 0) w1 += 8 - w1 % 8;
    x1 -= x1 % 8;
    
    epd_set_partial_ram_area_impl(driver, x1, y1, w1, h1);
    driver->write_command(driver, EPD_CMD_DISPLAY_UPDATE_CONTROL);
    driver->write_data(driver, 0xfc);
    driver->write_command(driver, EPD_CMD_MASTER_ACTIVATION);
    driver->wait_while_busy(driver, "refresh area", driver->partial_refresh_time);
    driver->power_is_on = true;
} 