#ifndef _GXEPD2_154_D67_H_
#define _GXEPD2_154_D67_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include "GxEPD2.h"

// Display dimensions and characteristics
#define GXEPD2_154_D67_WIDTH 200
#define GXEPD2_154_D67_WIDTH_VISIBLE GXEPD2_154_D67_WIDTH
#define GXEPD2_154_D67_HEIGHT 200
#define GXEPD2_154_D67_PANEL GxEPD2_GDEH0154D67
#define GXEPD2_154_D67_HAS_COLOR false
#define GXEPD2_154_D67_HAS_PARTIAL_UPDATE true
#define GXEPD2_154_D67_HAS_FAST_PARTIAL_UPDATE true

// Timing parameters
#define GXEPD2_154_D67_POWER_ON_TIME 100  // ms
#define GXEPD2_154_D67_POWER_OFF_TIME 150 // ms
#define GXEPD2_154_D67_FULL_REFRESH_TIME 2600 // ms
#define GXEPD2_154_D67_PARTIAL_REFRESH_TIME 500 // ms

// Forward declaration
typedef struct GxEPD2_154_D67 GxEPD2_154_D67_t;

// Function pointer types
typedef void (*write_command_fn)(GxEPD2_154_D67_t* driver, uint8_t command);
typedef void (*write_data_fn)(GxEPD2_154_D67_t* driver, uint8_t data);
typedef void (*wait_while_busy_fn)(GxEPD2_154_D67_t* driver, const char* msg, uint16_t timeout);
typedef void (*reset_fn)(GxEPD2_154_D67_t* driver);
typedef void (*refresh_fn)(GxEPD2_154_D67_t* driver, bool partial_update_mode);
typedef void (*power_off_fn)(GxEPD2_154_D67_t* driver);
typedef void (*hibernate_fn)(GxEPD2_154_D67_t* driver);
typedef void (*write_image_fn)(GxEPD2_154_D67_t* driver, const uint8_t* bitmap, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm);
typedef void (*write_image_part_fn)(GxEPD2_154_D67_t* driver, const uint8_t* bitmap, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm);
typedef void (*clear_screen_fn)(GxEPD2_154_D67_t* driver, uint8_t value);
typedef void (*write_screen_buffer_fn)(GxEPD2_154_D67_t* driver, uint8_t value);

// Driver struct (replaces the C++ class)
struct GxEPD2_154_D67 {
    // Device pointers
    const struct device* spi_dev;
    const struct device* gpio0_dev;
    const struct device* gpio1_dev;
    
    // Pin configuration
    int16_t cs_pin;
    int16_t dc_pin;
    int16_t rst_pin;
    int16_t busy_pin;
    
    // Display state
    bool power_is_on;
    bool using_partial_mode;
    bool hibernating;
    bool init_display_done;
    bool initial_refresh;
    bool initial_write;
    
    // Display buffer
    uint8_t* buffer;
    uint32_t buffer_size;
    
    // Function pointers (replaces virtual methods)
    write_command_fn write_command;
    write_data_fn write_data;
    wait_while_busy_fn wait_while_busy;
    reset_fn reset;
    refresh_fn refresh;
    power_off_fn power_off;
    hibernate_fn hibernate;
    write_image_fn write_image;
    write_image_part_fn write_image_part;
    clear_screen_fn clear_screen;
    write_screen_buffer_fn write_screen_buffer;
    
    // SPI configuration
    struct spi_config spi_cfg;
};

// Function declarations
GxEPD2_154_D67_t* GxEPD2_154_D67_init(int16_t cs, int16_t dc, int16_t rst, int16_t busy);
void GxEPD2_154_D67_deinit(GxEPD2_154_D67_t* driver);

// Core display functions
void GxEPD2_154_D67_clear_screen(GxEPD2_154_D67_t* driver, uint8_t value);
void GxEPD2_154_D67_write_screen_buffer(GxEPD2_154_D67_t* driver, uint8_t value);
void GxEPD2_154_D67_write_screen_buffer_again(GxEPD2_154_D67_t* driver, uint8_t value);
void GxEPD2_154_D67_write_image(GxEPD2_154_D67_t* driver, const uint8_t* bitmap, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm);
void GxEPD2_154_D67_write_image_for_full_refresh(GxEPD2_154_D67_t* driver, const uint8_t* bitmap, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm);
void GxEPD2_154_D67_write_image_part(GxEPD2_154_D67_t* driver, const uint8_t* bitmap, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm);
void GxEPD2_154_D67_write_image_again(GxEPD2_154_D67_t* driver, const uint8_t* bitmap, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm);
void GxEPD2_154_D67_write_image_part_again(GxEPD2_154_D67_t* driver, const uint8_t* bitmap, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm);
void GxEPD2_154_D67_refresh(GxEPD2_154_D67_t* driver, bool partial_update_mode);
void GxEPD2_154_D67_refresh_area(GxEPD2_154_D67_t* driver, int16_t x, int16_t y, int16_t w, int16_t h);
void GxEPD2_154_D67_power_off(GxEPD2_154_D67_t* driver);
void GxEPD2_154_D67_hibernate(GxEPD2_154_D67_t* driver);

#endif // _GXEPD2_154_D67_H_ 