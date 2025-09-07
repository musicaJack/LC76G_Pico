/**
 * @file ili9488_hal.cpp
 * @brief Hardware Abstraction Layer implementation for ILI9488
 * @note Simplified implementation for Raspberry Pi Pico
 */

#include "ili9488_hal.hpp"
#include "pin_config.hpp"
#include <algorithm>
#include <cstdio>
#include "hardware/pwm.h"


namespace ili9488 {
namespace hal {

// Static instance for singleton pattern
ILI9488HAL* ILI9488HAL::instance_ = nullptr;

// Constructor
ILI9488HAL::ILI9488HAL(const HardwareConfig& config) 
    : config_(config), is_initialized_(false) {
}

// Destructor
ILI9488HAL::~ILI9488HAL() {
    if (is_initialized_) {
        cleanup();
    }
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

// Initialize hardware
bool ILI9488HAL::initialize() {
    if (is_initialized_) {
        return true;
    }
    
    printf("Initializing ILI9488 HAL...\n");
    
    // Initialize SPI
    if (!initializeSPI()) {
        printf("Failed to initialize SPI\n");
        return false;
    }
    
    // Initialize GPIO pins
    if (!initializeGPIO()) {
        printf("Failed to initialize GPIO\n");
        return false;
    }
    

    
    is_initialized_ = true;
    printf("ILI9488 HAL initialized successfully\n");
    return true;
}

// Cleanup hardware
void ILI9488HAL::cleanup() {
    if (!is_initialized_) return;
    
    printf("Cleaning up ILI9488 HAL...\n");
    

    
    // Reset GPIO pins to input
    if (config_.pin_cs != 255) gpio_set_dir(config_.pin_cs, GPIO_IN);
    if (config_.pin_dc != 255) gpio_set_dir(config_.pin_dc, GPIO_IN);
    if (config_.pin_rst != 255) gpio_set_dir(config_.pin_rst, GPIO_IN);
    
    is_initialized_ = false;
    printf("ILI9488 HAL cleanup completed\n");
}

// Initialize SPI
bool ILI9488HAL::initializeSPI() {
    printf("Initializing SPI...\n");
    
    // Initialize SPI at specified speed
    uint32_t actual_speed = spi_init(config_.spi_inst, config_.spi_speed_hz);
    
    // Set SPI format: 8 bits, CPOL=0, CPHA=0, MSB first
    spi_set_format(config_.spi_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    // Set up SPI pins
    gpio_set_function(config_.pin_sck, GPIO_FUNC_SPI);
    gpio_set_function(config_.pin_mosi, GPIO_FUNC_SPI);
    if (config_.pin_miso != 255) {
        gpio_set_function(config_.pin_miso, GPIO_FUNC_SPI);
    }
    
    printf("SPI initialized at %lu Hz (requested %lu Hz)\n",
           (unsigned long)actual_speed, (unsigned long)config_.spi_speed_hz);
    
    return true;
}

// Initialize GPIO pins
bool ILI9488HAL::initializeGPIO() {
    printf("Initializing GPIO pins...\n");
    
    // Initialize CS pin
    if (config_.pin_cs != 255) {
        gpio_init(config_.pin_cs);
        gpio_set_dir(config_.pin_cs, GPIO_OUT);
        gpio_put(config_.pin_cs, 1);  // CS high (inactive)
    }
    
    // Initialize DC pin
    if (config_.pin_dc != 255) {
        gpio_init(config_.pin_dc);
        gpio_set_dir(config_.pin_dc, GPIO_OUT);
        gpio_put(config_.pin_dc, 0);  // DC low (command mode)
    }
    
    // Initialize RST pin
    if (config_.pin_rst != 255) {
        gpio_init(config_.pin_rst);
        gpio_set_dir(config_.pin_rst, GPIO_OUT);
        gpio_put(config_.pin_rst, 1);  // RST high (not in reset)
    }
    
    // Initialize backlight pin (PWM or direct)
    if (config_.pin_bl != 255) {
        #if ILI9488_BACKLIGHT_USE_PWM
        gpio_set_function(config_.pin_bl, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(config_.pin_bl);
        pwm_set_enabled(slice_num, true);
        setBacklightBrightness(ILI9488_BACKLIGHT_BRIGHTNESS);  // 使用配置的亮度
        #else
        gpio_init(config_.pin_bl);
        gpio_set_dir(config_.pin_bl, GPIO_OUT);
        gpio_put(config_.pin_bl, 1); // 常亮
        #endif
    }
    
    printf("GPIO pins initialized\n");
    return true;
}



// Hardware reset
void ILI9488HAL::hardwareReset() {
    if (config_.pin_rst == 255) return;
    
    printf("Performing hardware reset...\n");
    
    gpio_put(config_.pin_rst, 1);
    sleep_ms(10);
    gpio_put(config_.pin_rst, 0);
    sleep_ms(10);
    gpio_put(config_.pin_rst, 1);
    sleep_ms(150);
    
    printf("Hardware reset completed\n");
}

// Set chip select signal
void ILI9488HAL::setChipSelect(bool active) {
    if (config_.pin_cs != 255) {
        gpio_put(config_.pin_cs, active ? 0 : 1);
    }
}

// Set data/command signal
void ILI9488HAL::setDataCommand(bool is_data) {
    if (config_.pin_dc != 255) {
        gpio_put(config_.pin_dc, is_data ? 1 : 0);
    }
}

// Write command to display
void ILI9488HAL::writeCommand(uint8_t command) {
    setChipSelect(true);
    setDataCommand(false);  // Command mode
    spi_write_blocking(config_.spi_inst, &command, 1);
    setChipSelect(false);
}

// Write single data byte
void ILI9488HAL::writeData(uint8_t data) {
    setChipSelect(true);
    setDataCommand(true);   // Data mode
    spi_write_blocking(config_.spi_inst, &data, 1);
    setChipSelect(false);
}

// Write data buffer (blocking)
void ILI9488HAL::writeDataBuffer(const uint8_t* data, size_t length) {
    if (!data || length == 0) return;
    
    setChipSelect(true);
    setDataCommand(true);   // Data mode
    
    // Write in chunks for better performance
    size_t remaining = length;
    const uint8_t* ptr = data;
    
    while (remaining > 0) {
        size_t chunk_size = std::min(remaining, size_t(4096));
        spi_write_blocking(config_.spi_inst, ptr, chunk_size);
        ptr += chunk_size;
        remaining -= chunk_size;
    }
    
    setChipSelect(false);
}



// Set backlight brightness (0-255)
void ILI9488HAL::setBacklightBrightness(uint8_t brightness) {
    if (config_.pin_bl == 255) return;
    #if ILI9488_BACKLIGHT_USE_PWM
    uint slice_num = pwm_gpio_to_slice_num(config_.pin_bl);
    uint channel = pwm_gpio_to_channel(config_.pin_bl);
    pwm_set_chan_level(slice_num, channel, brightness);
    #else
    gpio_put(config_.pin_bl, brightness ? 1 : 0);
    #endif
}

// Enable/disable backlight
void ILI9488HAL::setBacklight(bool enable) {
    setBacklightBrightness(enable ? 255 : 0);
}

// Get singleton instance
ILI9488HAL& ILI9488HAL::getInstance(const HardwareConfig& config) {
    if (!instance_) {
        instance_ = new ILI9488HAL(config);
    }
    return *instance_;
}

// Timing utilities
void ILI9488HAL::delayMs(uint32_t ms) {
    sleep_ms(ms);
}

void ILI9488HAL::delayUs(uint32_t us) {
    sleep_us(us);
}

} // namespace hal
} // namespace ili9488 