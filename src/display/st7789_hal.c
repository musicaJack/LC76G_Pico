/**
 * @file st7789_hal.c
 * @brief ST7789 LCD Hardware Abstraction Layer implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "display/st7789_hal.h"

// Static configuration
static struct {
    spi_inst_t *spi_inst;
    uint8_t pin_cs;
    uint8_t pin_dc;
    uint8_t pin_reset;
    uint8_t pin_bl;
    bool is_initialized;
} st7789_hal_config = {0};

/**
 * @brief Initialize hardware interface
 */
bool st7789_hal_init(const st7789_config_t *config) {
    if (config == NULL) {
        printf("Error: HAL initialization config is NULL\n");
        return false;
    }
    
    printf("Initializing SPI and GPIO...\n");
    
    // Save configuration parameters
    st7789_hal_config.spi_inst = config->spi_inst;
    st7789_hal_config.pin_cs = config->pin_cs;
    st7789_hal_config.pin_dc = config->pin_dc;
    st7789_hal_config.pin_reset = config->pin_reset;
    st7789_hal_config.pin_bl = config->pin_bl;
    
    // Initialize reset pin and perform hardware reset
    gpio_init(config->pin_reset);
    gpio_set_dir(config->pin_reset, GPIO_OUT);
    gpio_put(config->pin_reset, 0);  // Reset state
    sleep_ms(100);
    gpio_put(config->pin_reset, 1);  // Release reset
    sleep_ms(100);
    
    // Initialize control pins
    gpio_init(config->pin_dc);
    gpio_init(config->pin_bl);
    gpio_init(config->pin_cs);
    
    // Set GPIO directions
    gpio_set_dir(config->pin_dc, GPIO_OUT);
    gpio_set_dir(config->pin_bl, GPIO_OUT);
    gpio_set_dir(config->pin_cs, GPIO_OUT);
    
    // Initial state - backlight off, CS high (not selected)
    gpio_put(config->pin_cs, 1);
    gpio_put(config->pin_dc, 1);
    gpio_put(config->pin_bl, 0);
    
    // Initialize SPI
    printf("Configuring SPI...\n");
    spi_init(config->spi_inst, config->spi_speed_hz);
    
    // Set SPI format - Mode 0
    spi_set_format(config->spi_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    // Set SPI pin functions
    gpio_set_function(config->pin_sck, GPIO_FUNC_SPI);
    gpio_set_function(config->pin_din, GPIO_FUNC_SPI);
    
    st7789_hal_config.is_initialized = true;
    printf("SPI and GPIO initialization complete\n");
    
    return true;
}

/**
 * @brief Send command to LCD
 */
void st7789_hal_write_cmd(uint8_t cmd) {
    if (!st7789_hal_config.is_initialized) return;
    
    gpio_put(st7789_hal_config.pin_cs, 0);  // Select chip
    gpio_put(st7789_hal_config.pin_dc, 0);  // Command mode
    spi_write_blocking(st7789_hal_config.spi_inst, &cmd, 1);
    gpio_put(st7789_hal_config.pin_cs, 1);  // Deselect chip
    sleep_us(10);  // Brief delay
}

/**
 * @brief Send single byte data to LCD
 */
void st7789_hal_write_data(uint8_t data) {
    if (!st7789_hal_config.is_initialized) return;
    
    gpio_put(st7789_hal_config.pin_cs, 0);  // Select chip
    gpio_put(st7789_hal_config.pin_dc, 1);  // Data mode
    spi_write_blocking(st7789_hal_config.spi_inst, &data, 1);
    gpio_put(st7789_hal_config.pin_cs, 1);  // Deselect chip
}

/**
 * @brief Send multiple bytes data to LCD
 */
void st7789_hal_write_data_bulk(const uint8_t *data, size_t len) {
    if (!st7789_hal_config.is_initialized || data == NULL) return;
    
    gpio_put(st7789_hal_config.pin_cs, 0);  // Select chip
    gpio_put(st7789_hal_config.pin_dc, 1);  // Data mode
    spi_write_blocking(st7789_hal_config.spi_inst, data, len);
    gpio_put(st7789_hal_config.pin_cs, 1);  // Deselect chip
}

/**
 * @brief Control LCD reset pin
 */
void st7789_hal_reset(bool state) {
    if (!st7789_hal_config.is_initialized) return;
    
    gpio_put(st7789_hal_config.pin_reset, state ? 1 : 0);
}

/**
 * @brief Control LCD backlight pin
 */
void st7789_hal_set_backlight(bool state) {
    if (!st7789_hal_config.is_initialized) return;
    
    gpio_put(st7789_hal_config.pin_bl, state ? 1 : 0);
}

/**
 * @brief Delay function (milliseconds)
 */
void st7789_hal_delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

/**
 * @brief Delay function (microseconds)
 */
void st7789_hal_delay_us(uint32_t us) {
    sleep_us(us);
} 