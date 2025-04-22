/**
 * @file st7789.c
 * @brief ST7789 LCD driver implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "display/st7789.h"
#include "display/st7789_hal.h"

// Static configuration
static struct {
    uint16_t width;
    uint16_t height;
    uint8_t rotation;
    bool is_initialized;
} st7789_config = {0};

/**
 * @brief ST7789 initialization sequence
 */
static void st7789_init_sequence(void) {
    printf("Running ST7789 initialization sequence...\n");
    
    // Wait after reset
    st7789_hal_delay_ms(120);
    
    // Basic initialization
    st7789_hal_write_cmd(0x01);    // Software Reset
    st7789_hal_delay_ms(120);      // Must wait after reset
    
    st7789_hal_write_cmd(0x11);    // Sleep Out Mode
    st7789_hal_delay_ms(120);      // Wait for sleep out
    
    // Memory Data Access Control
    st7789_hal_write_cmd(0x36);    // MADCTL
    st7789_hal_write_data(0x00);   // Set to normal orientation (may need adjustment)
    
    // Interface Pixel Format
    st7789_hal_write_cmd(0x3A);
    st7789_hal_write_data(0x55);   // 65K of RGB interface (16bit/pixel)
    
    // Display Inversion Control
    st7789_hal_write_cmd(0x21);    // Display Inversion On
    
    // Set address range
    st7789_hal_write_cmd(0x2A);    // Column Address Set
    st7789_hal_write_data(0x00);
    st7789_hal_write_data(0x00);
    st7789_hal_write_data((st7789_config.width >> 8) & 0xFF);
    st7789_hal_write_data(st7789_config.width & 0xFF);
    
    st7789_hal_write_cmd(0x2B);    // Row Address Set
    st7789_hal_write_data(0x00);
    st7789_hal_write_data(0x00);
    st7789_hal_write_data((st7789_config.height >> 8) & 0xFF);
    st7789_hal_write_data(st7789_config.height & 0xFF);
    
    // Additional settings - may need adjustment for different models
    st7789_hal_write_cmd(0xB2);    // Porch Setting
    st7789_hal_write_data(0x0C);
    st7789_hal_write_data(0x0C);
    st7789_hal_write_data(0x00);
    st7789_hal_write_data(0x33);
    st7789_hal_write_data(0x33);
    
    st7789_hal_write_cmd(0xB7);    // Gate Control
    st7789_hal_write_data(0x35);
    
    st7789_hal_write_cmd(0xBB);    // VCOM Setting
    st7789_hal_write_data(0x20);   // 0.9V
    
    st7789_hal_write_cmd(0xC0);    // LCM Control
    st7789_hal_write_data(0x2C);
    
    st7789_hal_write_cmd(0xC2);    // VDV and VRH Command Enable
    st7789_hal_write_data(0x01);
    
    st7789_hal_write_cmd(0xC3);    // VRH Set
    st7789_hal_write_data(0x12);
    
    st7789_hal_write_cmd(0xC4);    // VDV Set
    st7789_hal_write_data(0x20);
    
    st7789_hal_write_cmd(0xC6);    // Frame Rate Control in Normal Mode
    st7789_hal_write_data(0x0F);   // 60Hz
    
    st7789_hal_write_cmd(0xD0);    // Power Control 1
    st7789_hal_write_data(0xA4);
    st7789_hal_write_data(0xA1);
    
    st7789_hal_write_cmd(0xE0);    // Positive Voltage Gamma Control
    st7789_hal_write_data(0xD0);
    st7789_hal_write_data(0x08);
    st7789_hal_write_data(0x11);
    st7789_hal_write_data(0x08);
    st7789_hal_write_data(0x0C);
    st7789_hal_write_data(0x15);
    st7789_hal_write_data(0x39);
    st7789_hal_write_data(0x33);
    st7789_hal_write_data(0x50);
    st7789_hal_write_data(0x36);
    st7789_hal_write_data(0x13);
    st7789_hal_write_data(0x14);
    st7789_hal_write_data(0x29);
    st7789_hal_write_data(0x2D);
    
    st7789_hal_write_cmd(0xE1);    // Negative Voltage Gamma Control
    st7789_hal_write_data(0xD0);
    st7789_hal_write_data(0x08);
    st7789_hal_write_data(0x10);
    st7789_hal_write_data(0x08);
    st7789_hal_write_data(0x06);
    st7789_hal_write_data(0x06);
    st7789_hal_write_data(0x39);
    st7789_hal_write_data(0x44);
    st7789_hal_write_data(0x51);
    st7789_hal_write_data(0x0B);
    st7789_hal_write_data(0x16);
    st7789_hal_write_data(0x14);
    st7789_hal_write_data(0x2F);
    st7789_hal_write_data(0x31);
    
    st7789_hal_delay_ms(10);
    
    // Turn on display
    st7789_hal_write_cmd(0x29);    // Display On
    st7789_hal_delay_ms(10);
    
    printf("ST7789 initialization sequence completed\n");
}

/**
 * @brief Initialize ST7789 driver
 */
bool st7789_init(const st7789_config_t *config) {
    if (config == NULL) {
        printf("Error: Driver initialization config is NULL\n");
        return false;
    }
    
    printf("Starting LCD initialization...\n");
    
    // Save screen parameters
    st7789_config.width = config->width;
    st7789_config.height = config->height;
    st7789_config.rotation = config->rotation;
    
    // Initialize hardware abstraction layer
    if (!st7789_hal_init(config)) {
        printf("Error: Hardware abstraction layer initialization failed\n");
        return false;
    }
    
    // Hardware reset
    printf("Performing hardware reset...\n");
    st7789_hal_reset(false);   // Reset state
    st7789_hal_delay_ms(200);  // Keep low for 200ms
    st7789_hal_reset(true);    // Release reset
    st7789_hal_delay_ms(200);  // Wait after reset
    
    // Run initialization sequence
    st7789_init_sequence();
    
    // Set rotation
    st7789_set_rotation(config->rotation);
    
    st7789_config.is_initialized = true;
    printf("LCD initialization complete\n");
    
    return true;
}

/**
 * @brief Set LCD backlight
 */
void st7789_set_backlight(bool on) {
    if (!st7789_config.is_initialized) return;
    
    st7789_hal_set_backlight(on);
}

/**
 * @brief Set display orientation
 */
void st7789_set_rotation(uint8_t rotation) {
    if (!st7789_config.is_initialized) return;
    
    // Set display orientation
    uint8_t madctl = 0;
    uint16_t old_width, old_height;
    
    // Save original dimensions
    old_width = st7789_config.width;
    old_height = st7789_config.height;
    
    // Get native width and height (without rotation)
    uint16_t native_width, native_height;
    if (st7789_config.rotation == 0 || st7789_config.rotation == 2) {
        // Current rotation is 0 or 180 degrees
        native_width = old_width;
        native_height = old_height;
    } else {
        // Current rotation is 90 or 270 degrees
        native_width = old_height;
        native_height = old_width;
    }
    
    // Set MADCTL and dimensions based on new rotation angle
    switch (rotation & 0x03) {
        case 0:  // 0 degree rotation
            madctl = 0x00;
            st7789_config.width = native_width;
            st7789_config.height = native_height;
            break;
        case 1:  // 90 degree rotation
            madctl = 0x60;
            st7789_config.width = native_height;
            st7789_config.height = native_width;
            break;
        case 2:  // 180 degree rotation
            madctl = 0xC0;
            st7789_config.width = native_width;
            st7789_config.height = native_height;
            break;
        case 3:  // 270 degree rotation
            madctl = 0xA0;
            st7789_config.width = native_height;
            st7789_config.height = native_width;
            break;
    }
    
    // Set MADCTL register
    st7789_hal_write_cmd(0x36);
    st7789_hal_write_data(madctl);
    
    // Update current rotation state
    st7789_config.rotation = rotation & 0x03;
    
    // If dimensions have changed, reset the screen window
    if (old_width != st7789_config.width || old_height != st7789_config.height) {
        printf("Display dimensions after rotation: %d x %d\n", 
               st7789_config.width, st7789_config.height);
        
        // Set new address window range
        st7789_hal_write_cmd(0x2A);    // Column Address Set
        st7789_hal_write_data(0x00);
        st7789_hal_write_data(0x00);
        st7789_hal_write_data((st7789_config.width >> 8) & 0xFF);
        st7789_hal_write_data(st7789_config.width & 0xFF);
        
        st7789_hal_write_cmd(0x2B);    // Row Address Set
        st7789_hal_write_data(0x00);
        st7789_hal_write_data(0x00);
        st7789_hal_write_data((st7789_config.height >> 8) & 0xFF);
        st7789_hal_write_data(st7789_config.height & 0xFF);
    }
}

/**
 * @brief Set drawing window
 */
void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    if (!st7789_config.is_initialized) return;
    
    // Boundary check
    if (x0 >= st7789_config.width) x0 = st7789_config.width - 1;
    if (y0 >= st7789_config.height) y0 = st7789_config.height - 1;
    if (x1 >= st7789_config.width) x1 = st7789_config.width - 1;
    if (y1 >= st7789_config.height) y1 = st7789_config.height - 1;
    
    // Set column address
    st7789_hal_write_cmd(0x2A);
    st7789_hal_write_data(x0 >> 8);
    st7789_hal_write_data(x0 & 0xFF);
    st7789_hal_write_data(x1 >> 8);
    st7789_hal_write_data(x1 & 0xFF);
    
    // Set row address
    st7789_hal_write_cmd(0x2B);
    st7789_hal_write_data(y0 >> 8);
    st7789_hal_write_data(y0 & 0xFF);
    st7789_hal_write_data(y1 >> 8);
    st7789_hal_write_data(y1 & 0xFF);
    
    // Prepare to write to RAM
    st7789_hal_write_cmd(0x2C);
}

/**
 * @brief Draw a single pixel
 */
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (!st7789_config.is_initialized) return;
    
    // Boundary check
    if (x >= st7789_config.width || y >= st7789_config.height) return;
    
    // Set drawing window to a single pixel
    st7789_set_window(x, y, x, y);
    
    // Write color data
    uint8_t data[2] = {color >> 8, color & 0xFF};
    st7789_hal_write_data_bulk(data, 2);
}

/**
 * @brief Fill the entire screen with specified color
 */
void st7789_fill_screen(uint16_t color) {
    if (!st7789_config.is_initialized) return;
    
    // Set drawing window to entire screen
    st7789_set_window(0, 0, st7789_config.width - 1, st7789_config.height - 1);
    
    // Prepare color data
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint8_t data[2] = {hi, lo};
    
    // Fill pixels
    for (uint32_t i = 0; i < (uint32_t)st7789_config.width * st7789_config.height; i++) {
        st7789_hal_write_data_bulk(data, 2);
    }
}

/**
 * @brief Send block data to LCD
 */
void st7789_write_data_buffer(const uint8_t *data, size_t len) {
    if (!st7789_config.is_initialized || data == NULL) return;
    
    st7789_hal_write_data_bulk(data, len);
} 