/**
 * @file st7789_hal.h
 * @brief ST7789 LCD Hardware Abstraction Layer header file
 */

#ifndef _ST7789_HAL_H_
#define _ST7789_HAL_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "st7789.h"

/**
 * @brief Initialize hardware interface
 * 
 * @param config LCD configuration parameters
 * @return bool Whether initialization was successful
 */
bool st7789_hal_init(const st7789_config_t *config);

/**
 * @brief Send command to LCD
 * 
 * @param cmd Command value
 */
void st7789_hal_write_cmd(uint8_t cmd);

/**
 * @brief Send single byte data to LCD
 * 
 * @param data Data value
 */
void st7789_hal_write_data(uint8_t data);

/**
 * @brief Send multiple byte data to LCD
 * 
 * @param data Data buffer pointer
 * @param len Data length
 */
void st7789_hal_write_data_bulk(const uint8_t *data, size_t len);

/**
 * @brief Control LCD reset pin
 * 
 * @param state Reset state (true for high level, false for low level)
 */
void st7789_hal_reset(bool state);

/**
 * @brief Control LCD backlight pin
 * 
 * @param state Backlight state (true for on, false for off)
 */
void st7789_hal_set_backlight(bool state);

/**
 * @brief Delay function (milliseconds)
 * 
 * @param ms Milliseconds to delay
 */
void st7789_hal_delay_ms(uint32_t ms);

/**
 * @brief Delay function (microseconds)
 * 
 * @param us Microseconds to delay
 */
void st7789_hal_delay_us(uint32_t us);

#endif /* _ST7789_HAL_H_ */ 