/**
 * @file st7789_gfx.c
 * @brief ST7789 LCD Graphics Library Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "display/st7789_gfx.h"
#include "display/st7789.h"

/**
 * @brief Draw horizontal line
 */
void st7789_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    // Set drawing window
    st7789_set_window(x, y, x + w - 1, y);
    
    // Prepare color data
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint8_t data[2] = {hi, lo};
    
    // Draw pixels
    for (uint16_t i = 0; i < w; i++) {
        st7789_write_data_buffer(data, 2);
    }
}

/**
 * @brief Draw vertical line
 */
void st7789_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color) {
    // Set drawing window
    st7789_set_window(x, y, x, y + h - 1);
    
    // Prepare color data
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint8_t data[2] = {hi, lo};
    
    // Draw pixels
    for (uint16_t i = 0; i < h; i++) {
        st7789_write_data_buffer(data, 2);
    }
}

/**
 * @brief Draw line (Bresenham algorithm)
 */
void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    
    if (steep) {
        // Swap x0 and y0
        uint16_t temp = x0;
        x0 = y0;
        y0 = temp;
        
        // Swap x1 and y1
        temp = x1;
        x1 = y1;
        y1 = temp;
    }
    
    if (x0 > x1) {
        // Swap x0 and x1
        uint16_t temp = x0;
        x0 = x1;
        x1 = temp;
        
        // Swap y0 and y1
        temp = y0;
        y0 = y1;
        y1 = temp;
    }
    
    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep;
    
    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }
    
    for (; x0 <= x1; x0++) {
        if (steep) {
            st7789_draw_pixel(y0, x0, color);
        } else {
            st7789_draw_pixel(x0, y0, color);
        }
        
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 * @brief Draw rectangle
 */
void st7789_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // Draw horizontal lines
    st7789_draw_hline(x, y, w, color);
    st7789_draw_hline(x, y + h - 1, w, color);
    
    // Draw vertical lines
    st7789_draw_vline(x, y, h, color);
    st7789_draw_vline(x + w - 1, y, h, color);
}

/**
 * @brief Draw filled rectangle
 */
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // Set drawing window
    st7789_set_window(x, y, x + w - 1, y + h - 1);
    
    // Prepare color data
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint8_t data[2] = {hi, lo};
    
    // Fill pixels
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        st7789_write_data_buffer(data, 2);
    }
}

/**
 * @brief Draw circle (Bresenham algorithm)
 */
void st7789_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    st7789_draw_pixel(x0, y0 + r, color);
    st7789_draw_pixel(x0, y0 - r, color);
    st7789_draw_pixel(x0 + r, y0, color);
    st7789_draw_pixel(x0 - r, y0, color);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        st7789_draw_pixel(x0 + x, y0 + y, color);
        st7789_draw_pixel(x0 - x, y0 + y, color);
        st7789_draw_pixel(x0 + x, y0 - y, color);
        st7789_draw_pixel(x0 - x, y0 - y, color);
        st7789_draw_pixel(x0 + y, y0 + x, color);
        st7789_draw_pixel(x0 - y, y0 + x, color);
        st7789_draw_pixel(x0 + y, y0 - x, color);
        st7789_draw_pixel(x0 - y, y0 - x, color);
    }
}

/**
 * @brief Draw filled circle
 */
void st7789_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    // Use horizontal lines to draw filled circle
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    // Draw middle horizontal line
    st7789_draw_hline(x0 - r, y0, 2 * r + 1, color);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        // Draw top and bottom horizontal line pairs
        st7789_draw_hline(x0 - x, y0 + y, 2 * x + 1, color);
        st7789_draw_hline(x0 - x, y0 - y, 2 * x + 1, color);
        
        // Draw left and right horizontal line pairs
        st7789_draw_hline(x0 - y, y0 + x, 2 * y + 1, color);
        st7789_draw_hline(x0 - y, y0 - x, 2 * y + 1, color);
    }
} 