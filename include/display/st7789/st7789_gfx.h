/**
 * @file st7789_gfx.h
 * @brief ST7789 LCD Graphics Library header file
 */

#ifndef _ST7789_GFX_H_
#define _ST7789_GFX_H_

#include <stdint.h>
#include <stdbool.h>
#include "st7789.h"

/**
 * @brief Color conversion macro (R,G,B to RGB565)
 */
#define ST7789_COLOR_RGB(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

/**
 * @brief Draw horizontal line
 * 
 * @param x Starting X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param color Color
 */
void st7789_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);

/**
 * @brief Draw vertical line
 * 
 * @param x X coordinate
 * @param y Starting Y coordinate
 * @param h Height
 * @param color Color
 */
void st7789_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);

/**
 * @brief Draw line
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param color Color
 */
void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief Draw rectangle
 * 
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param w Width
 * @param h Height
 * @param color Color
 */
void st7789_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Draw filled rectangle
 * 
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param w Width
 * @param h Height
 * @param color Color
 */
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Draw circle
 * 
 * @param x0 Center X coordinate
 * @param y0 Center Y coordinate
 * @param r Radius
 * @param color Color
 */
void st7789_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * @brief Draw filled circle
 * 
 * @param x0 Center X coordinate
 * @param y0 Center Y coordinate
 * @param r Radius
 * @param color Color
 */
void st7789_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * @brief Draw single character
 * 
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param c Character
 * @param color Character color
 * @param bg Background color
 * @param size Font size (1=8x8, 2=16x16, etc.)
 */
void st7789_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);

/**
 * @brief Draw string
 * 
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param str String pointer
 * @param color Character color
 * @param bg Background color
 * @param size Font size
 */
void st7789_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);

#endif /* _ST7789_GFX_H_ */ 