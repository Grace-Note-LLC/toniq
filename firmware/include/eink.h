#ifndef EINK_H
#define EINK_H

#include <stdint.h>

// Initialize the e-ink display
void eink_init(void);

// Update the entire display with a new image
void eink_update_full(const uint8_t* image_data);

// Update a portion of the display
void eink_update_partial(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t* image_data);

// Draw text on the display
void eink_draw_text(int16_t x, int16_t y, const char* text);

// Power off the display
void eink_power_off(void);

// Deep sleep mode
void eink_deep_sleep(void);

#endif // EINK_H 