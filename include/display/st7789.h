/**
 * @file st7789.h
 * @brief ST7789 LCD driver header file
 */

#ifndef _ST7789_H_
#define _ST7789_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

/**
 * @brief LCD configuration structure
 */
typedef struct {
    spi_inst_t *spi_inst;    // SPI instance
    uint32_t spi_speed_hz;   // SPI speed (Hz)
    
    // Pin definitions
    uint8_t pin_din;         // MOSI
    uint8_t pin_sck;         // SCK
    uint8_t pin_cs;          // CS
    uint8_t pin_dc;          // Data/Command
    uint8_t pin_reset;       // Reset
    uint8_t pin_bl;          // Backlight
    
    // Screen parameters
    uint16_t width;          // Width
    uint16_t height;         // Height
    
    // Orientation
    uint8_t rotation;        // Rotation
} st7789_config_t;

/**
 * @brief Color definitions (RGB565 format)
 */
#define ST7789_BLACK       0x0000
#define ST7789_WHITE       0xFFFF
#define ST7789_RED         0xF800
#define ST7789_GREEN       0x07E0
#define ST7789_BLUE        0x001F
#define ST7789_YELLOW      0xFFE0
#define ST7789_CYAN        0x07FF
#define ST7789_MAGENTA     0xF81F

/**
 * @brief Initialize ST7789 driver
 * 
 * @param config LCD configuration parameters
 * @return bool Whether initialization was successful
 */
bool st7789_init(const st7789_config_t *config);

/**
 * @brief Set LCD backlight
 * 
 * @param on Backlight state (true for on, false for off)
 */
void st7789_set_backlight(bool on);

/**
 * @brief Fill entire screen with specified color
 * 
 * @param color Fill color (RGB565 format)
 */
void st7789_fill_screen(uint16_t color);

/**
 * @brief Set drawing window
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 */
void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * @brief Draw a single pixel
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Pixel color
 */
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Send block data to LCD
 * 
 * @param data Data pointer
 * @param len Data length
 */
void st7789_write_data_buffer(const uint8_t *data, size_t len);

/**
 * @brief Set display orientation
 * 
 * @param rotation Rotation value (0-3)
 */
void st7789_set_rotation(uint8_t rotation);

#endif /* _ST7789_H_ */ 