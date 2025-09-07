/**
 * @file ili9488_hal_simple.hpp
 * @brief 简化的ILI9488硬件抽象层
 * @note 移除了复杂的DMA功能，专注于基本SPI通信
 */

#pragma once

#include <cstdint>
#include <memory>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pin_config.hpp"

namespace ili9488 {
namespace hal {

/**
 * @brief Hardware configuration structure
 */
struct HardwareConfig {
    // SPI configuration
    spi_inst_t* spi_inst = spi0;          ///< SPI instance (spi0 or spi1)
    uint32_t spi_speed_hz = 40000000;     ///< SPI speed in Hz (40MHz default)
    
    // GPIO pin assignments
    uint8_t pin_sck = 0;                  ///< SPI clock pin
    uint8_t pin_mosi = 0;                 ///< SPI MOSI pin
    uint8_t pin_miso = 255;               ///< SPI MISO pin (not used, 255 = disabled)
    uint8_t pin_cs = 0;                   ///< Chip select pin
    uint8_t pin_dc = 0;                   ///< Data/Command control pin
    uint8_t pin_rst = 0;                  ///< Reset pin
    uint8_t pin_bl = 0;                   ///< Backlight control pin
    
    // 默认构造函数
    HardwareConfig() = default;
};

/**
 * @brief Hardware abstraction layer for ILI9488
 * @note Implements RAII pattern for resource management
 */
class ILI9488HAL {
public:
    /**
     * @brief Construct HAL with hardware configuration
     * @param config Hardware pin and interface configuration
     */
    explicit ILI9488HAL(const HardwareConfig& config);
    
    /**
     * @brief Destructor - automatically cleans up resources
     */
    ~ILI9488HAL();
    
    // 禁用拷贝构造和赋值
    ILI9488HAL(const ILI9488HAL&) = delete;
    ILI9488HAL& operator=(const ILI9488HAL&) = delete;
    
    /**
     * @brief Get singleton instance
     * @param config Hardware configuration
     * @return Pointer to singleton instance
     */
    static ILI9488HAL* getInstance(const HardwareConfig& config);
    
    /**
     * @brief Initialize hardware interface
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Cleanup hardware resources
     */
    void cleanup();
    
    /**
     * @brief Reset display
     */
    void reset();
    
    /**
     * @brief Write command to display
     * @param cmd Command byte
     */
    void writeCommand(uint8_t cmd);
    
    /**
     * @brief Write single data byte to display
     * @param data Data byte
     */
    void writeData(uint8_t data);
    
    /**
     * @brief Write data buffer to display
     * @param data Pointer to data buffer
     * @param length Number of bytes to write
     */
    void writeDataBuffer(const uint8_t* data, size_t length);
    
    /**
     * @brief Set backlight on/off
     * @param enable true to enable, false to disable
     */
    void setBacklight(bool enable);
    
    /**
     * @brief Set backlight brightness
     * @param brightness Brightness level (0-255)
     */
    void setBacklightBrightness(uint8_t brightness);
    
    /**
     * @brief Check if HAL is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;
    
    /**
     * @brief Get hardware configuration
     * @return Reference to hardware configuration
     */
    const HardwareConfig& getConfig() const;

private:
    // Hardware configuration
    HardwareConfig config_;
    
    // Initialization state
    bool is_initialized_;
    
    // Static instance for singleton pattern
    static ILI9488HAL* instance_;
    
    // Private helper methods
    void initializeGPIO();
    bool initializeSPI();
    void initializeBacklight();
};

} // namespace hal
} // namespace ili9488
