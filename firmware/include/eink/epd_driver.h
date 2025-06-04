#ifndef EPD_DRIVER_H
#define EPD_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

// Display dimensions
#define EPD_WIDTH 200
#define EPD_HEIGHT 200
#define EPD_BUFFER_SIZE ((EPD_WIDTH * EPD_HEIGHT) / 8)

// Display commands
#define EPD_CMD_POWER_SETTING 0x01
#define EPD_CMD_POWER_ON 0x04
#define EPD_CMD_PANEL_SETTING 0x00
#define EPD_CMD_DISPLAY_REFRESH 0x12
#define EPD_CMD_DEEP_SLEEP 0x10
#define EPD_CMD_DATA_START_TRANSMISSION 0x13
#define EPD_CMD_DISPLAY_UPDATE_CONTROL 0x22
#define EPD_CMD_MASTER_ACTIVATION 0x20

// Forward declaration of the driver struct
typedef struct epd_driver epd_driver_t;

// Display struct (replaces the C++ class)
struct epd_driver {
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
    void (*write_command)(epd_driver_t* driver, uint8_t command);
    void (*write_data)(epd_driver_t* driver, uint8_t data);
    void (*wait_while_busy)(epd_driver_t* driver, const char* msg, uint16_t timeout);
    void (*reset)(epd_driver_t* driver);
    void (*refresh)(epd_driver_t* driver, bool partial_update_mode);
    void (*power_off)(epd_driver_t* driver);
    void (*hibernate)(epd_driver_t* driver);
    
    // Timing parameters
    uint16_t power_on_time;
    uint16_t power_off_time;
    uint16_t full_refresh_time;
    uint16_t partial_refresh_time;

    // SPI configuration
    struct spi_config spi_config;
};

// Function declarations (replaces class methods)

// Initialization
int epd_init(epd_driver_t* driver, int16_t cs_pin, int16_t dc_pin, int16_t rst_pin, int16_t busy_pin,
             const struct device* gpio0_dev, const struct device* gpio1_dev, const struct device* spi_dev);
void epd_deinit(epd_driver_t* driver);

// Display operations
void epd_clear_screen(epd_driver_t* driver, uint8_t value);
void epd_write_screen_buffer(epd_driver_t* driver, uint8_t value);
void epd_write_image(epd_driver_t* driver, const uint8_t* bitmap, int16_t x, int16_t y, 
                    int16_t w, int16_t h, bool invert, bool mirror_y);
void epd_write_image_part(epd_driver_t* driver, const uint8_t* bitmap, 
                         int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                         int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y);

// Display updates
void epd_refresh(epd_driver_t* driver, bool partial_update_mode);
void epd_refresh_area(epd_driver_t* driver, int16_t x, int16_t y, int16_t w, int16_t h);

// Power management
void epd_power_on(epd_driver_t* driver);
void epd_power_off(epd_driver_t* driver);
void epd_hibernate(epd_driver_t* driver);

// Helper functions
void epd_set_partial_ram_area(epd_driver_t* driver, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void epd_set_rotation(epd_driver_t* driver, uint8_t rotation);
void epd_set_text_color(epd_driver_t* driver, uint16_t color);
void epd_set_font(epd_driver_t* driver, const void* font);

#endif // EPD_DRIVER_H 