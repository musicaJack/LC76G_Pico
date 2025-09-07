/**
 * @file ili9488_hal_simple.cpp
 * @brief 简化的ILI9488硬件抽象层实现
 * @note 移除了复杂的DMA功能，专注于基本SPI通信
 */

#include "ili9488_hal.hpp"
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

// Get singleton instance
ILI9488HAL* ILI9488HAL::getInstance(const HardwareConfig& config) {
    if (instance_ == nullptr) {
        instance_ = new ILI9488HAL(config);
    }
    return instance_;
}

// Initialize hardware
bool ILI9488HAL::initialize() {
    if (is_initialized_) {
        return true;
    }
    
    printf("  [ILI9488HAL] 初始化硬件抽象层...\n");
    
    // Initialize GPIO pins
    initializeGPIO();
    
    // Initialize SPI
    if (!initializeSPI()) {
        printf("  [ILI9488HAL] SPI初始化失败\n");
        return false;
    }
    
    // Initialize backlight
    initializeBacklight();
    
    is_initialized_ = true;
    printf("  [ILI9488HAL] 硬件抽象层初始化成功\n");
    return true;
}

// Cleanup hardware
void ILI9488HAL::cleanup() {
    if (!is_initialized_) {
        return;
    }
    
    printf("  [ILI9488HAL] 清理硬件资源...\n");
    
    // Turn off backlight
    setBacklight(false);
    
    // Reset display
    reset();
    
    is_initialized_ = false;
}

// Initialize GPIO pins
void ILI9488HAL::initializeGPIO() {
    printf("  [ILI9488HAL] 配置GPIO引脚...\n");
    
    // Configure control pins
    gpio_init(config_.pin_dc);
    gpio_set_dir(config_.pin_dc, GPIO_OUT);
    gpio_put(config_.pin_dc, 0);
    
    gpio_init(config_.pin_rst);
    gpio_set_dir(config_.pin_rst, GPIO_OUT);
    gpio_put(config_.pin_rst, 1);
    
    gpio_init(config_.pin_cs);
    gpio_set_dir(config_.pin_cs, GPIO_OUT);
    gpio_put(config_.pin_cs, 1);
    
    // Configure backlight pin
    gpio_init(config_.pin_bl);
    gpio_set_dir(config_.pin_bl, GPIO_OUT);
    gpio_put(config_.pin_bl, 0);
}

// Initialize SPI
bool ILI9488HAL::initializeSPI() {
    printf("  [ILI9488HAL] 初始化SPI接口...\n");
    
    // Initialize SPI
    spi_init(config_.spi_inst, config_.spi_speed_hz);
    
    // Configure SPI pins
    gpio_set_function(config_.pin_sck, GPIO_FUNC_SPI);
    gpio_set_function(config_.pin_mosi, GPIO_FUNC_SPI);
    
    printf("  [ILI9488HAL] SPI初始化完成，速度: %lu Hz\n", config_.spi_speed_hz);
    return true;
}

// Initialize backlight
void ILI9488HAL::initializeBacklight() {
    printf("  [ILI9488HAL] 初始化背光控制...\n");
    
    // Simple GPIO backlight control
    gpio_put(config_.pin_bl, 1);
    printf("  [ILI9488HAL] 背光已开启\n");
}

// Reset display
void ILI9488HAL::reset() {
    printf("  [ILI9488HAL] 复位显示屏...\n");
    
    gpio_put(config_.pin_rst, 0);
    sleep_ms(10);
    gpio_put(config_.pin_rst, 1);
    sleep_ms(120);
}

// Write command
void ILI9488HAL::writeCommand(uint8_t cmd) {
    gpio_put(config_.pin_cs, 0);
    gpio_put(config_.pin_dc, 0);  // Command mode
    spi_write_blocking(config_.spi_inst, &cmd, 1);
    gpio_put(config_.pin_cs, 1);
}

// Write data
void ILI9488HAL::writeData(uint8_t data) {
    gpio_put(config_.pin_cs, 0);
    gpio_put(config_.pin_dc, 1);  // Data mode
    spi_write_blocking(config_.spi_inst, &data, 1);
    gpio_put(config_.pin_cs, 1);
}

// Write data buffer
void ILI9488HAL::writeDataBuffer(const uint8_t* data, size_t length) {
    if (!data || length == 0) return;
    
    gpio_put(config_.pin_cs, 0);
    gpio_put(config_.pin_dc, 1);  // Data mode
    
    // Write in chunks for better performance
    size_t remaining = length;
    const uint8_t* ptr = data;
    
    while (remaining > 0) {
        size_t chunk_size = std::min(remaining, size_t(1024));
        spi_write_blocking(config_.spi_inst, ptr, chunk_size);
        ptr += chunk_size;
        remaining -= chunk_size;
    }
    
    gpio_put(config_.pin_cs, 1);
}

// Set backlight
void ILI9488HAL::setBacklight(bool enable) {
    gpio_put(config_.pin_bl, enable ? 1 : 0);
}

// Set backlight brightness (simplified - just on/off)
void ILI9488HAL::setBacklightBrightness(uint8_t brightness) {
    setBacklight(brightness > 0);
}

// Check if initialized
bool ILI9488HAL::isInitialized() const {
    return is_initialized_;
}

// Get configuration
const HardwareConfig& ILI9488HAL::getConfig() const {
    return config_;
}

} // namespace hal
} // namespace ili9488
