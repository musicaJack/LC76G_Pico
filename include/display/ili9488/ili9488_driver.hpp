#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include "pico/stdlib.h"
#include "hardware/spi.h"

namespace ili9488 {

/**
 * @brief Color mode enumeration
 */
enum class ColorMode {
    RGB565,    // 16-bit color
    RGB666,    // 18-bit color (native ILI9488)
    RGB888     // 24-bit color
};

/**
 * @brief Font layout enumeration
 */
enum class FontLayout {
    Horizontal, // Horizontal dot matrix: one byte per column
    Vertical   // Vertical dot matrix: one byte per row
};

/**
 * @brief Rotation enumeration
 */
enum class Rotation {
    Portrait_0 = 0,     // 0°
    Landscape_90 = 1,   // 90°
    Portrait_180 = 2,   // 180°
    Landscape_270 = 3   // 270°
};

/**
 * @brief ILI9488 TFT LCD Driver Class
 * 
 * Modern C++ driver for ILI9488 3.5" 320x480 TFT LCD display.
 * Supports RGB666 (18-bit) native color mode with RGB565 and RGB888 compatibility.
 */
class ILI9488Driver {
public:
    // Display parameters
    static constexpr uint16_t LCD_WIDTH = 320;
    static constexpr uint16_t LCD_HEIGHT = 480;
    
    // Color constants (RGB666 format)
    static constexpr uint32_t COLOR_RED = 0xFC0000;
    static constexpr uint32_t COLOR_GREEN = 0x00FC00;
    static constexpr uint32_t COLOR_BLUE = 0x0000FC;
    static constexpr uint32_t COLOR_WHITE = 0xFCFCFC;
    static constexpr uint32_t COLOR_BLACK = 0x000000;
    static constexpr uint32_t COLOR_YELLOW = 0xFCFC00;
    static constexpr uint32_t COLOR_CYAN = 0x00FCFC;
    static constexpr uint32_t COLOR_MAGENTA = 0xFC00FC;

public:
    /**
     * @brief Constructor
     * 
     * @param spi_inst SPI instance (spi0 or spi1)
     * @param pin_dc Data/Command pin
     * @param pin_rst Reset pin
     * @param pin_cs Chip Select pin
     * @param pin_sck SPI Clock pin
     * @param pin_mosi SPI MOSI pin
     * @param pin_bl Backlight pin
     * @param spi_speed_hz SPI speed in Hz (default: 40MHz)
     */
    ILI9488Driver(spi_inst_t* spi_inst, uint8_t pin_dc, uint8_t pin_rst, uint8_t pin_cs,
                  uint8_t pin_sck, uint8_t pin_mosi, uint8_t pin_bl, 
                  uint32_t spi_speed_hz = 40000000);
    
    /**
     * @brief Destructor
     */
    ~ILI9488Driver();

    // Non-copyable, non-movable
    ILI9488Driver(const ILI9488Driver&) = delete;
    ILI9488Driver& operator=(const ILI9488Driver&) = delete;
    ILI9488Driver(ILI9488Driver&&) = delete;
    ILI9488Driver& operator=(ILI9488Driver&&) = delete;

public:
    // === Initialization and Basic Control ===
    
    /**
     * @brief Initialize the display
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Reset the display hardware
     */
    void reset();
    
    /**
     * @brief Clear the display buffer
     */
    void clear();
    
    /**
     * @brief Clear the display and fill with color
     */
    void clearDisplay();

public:
    // === Pixel Operations ===
    
    /**
     * @brief Draw a single pixel (RGB565)
     */
    void drawPixel(uint16_t x, uint16_t y, uint16_t color565);
    
    /**
     * @brief Draw a single pixel (RGB888/24-bit)
     */
    void drawPixelRGB24(uint16_t x, uint16_t y, uint32_t color24);
    
    /**
     * @brief Draw a single pixel (RGB666/18-bit native)
     */
    void drawPixelRGB666(uint16_t x, uint16_t y, uint32_t color666);

public:
    // === Batch Pixel Operations ===
    
    /**
     * @brief Write multiple pixels (RGB565)
     */
    void writePixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, 
                     const uint16_t* colors, size_t count);
    
    /**
     * @brief Write multiple pixels (RGB888)
     */
    void writePixelsRGB24(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                          const uint32_t* colors, size_t count);

public:
    // === Fill Methods ===
    
    /**
     * @brief Fill rectangular area with color (RGB565)
     * @param x0 Start X coordinate
     * @param y0 Start Y coordinate
     * @param x1 End X coordinate
     * @param y1 End Y coordinate
     * @param color RGB565 color value
     */
    void fillArea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
    
    /**
     * @brief Fill rectangular area with color (RGB666)
     * @param x0 Start X coordinate
     * @param y0 Start Y coordinate
     * @param x1 End X coordinate
     * @param y1 End Y coordinate
     * @param color666 RGB666 color value
     */
    void fillAreaRGB666(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color666);
    
    /**
     * @brief Fill entire screen with color (RGB565)
     * @param color RGB565 color value
     */
    void fillScreen(uint16_t color);
    
    /**
     * @brief Fill entire screen with color (RGB666)
     * @param color666 RGB666 color value
     */
    void fillScreenRGB666(uint32_t color666);

public:
    // === Display Control ===
    
    /**
     * @brief Set display rotation
     */
    void setRotation(Rotation rotation);
    
    /**
     * @brief Get current rotation
     */
    Rotation getRotation() const;
    
    /**
     * @brief Control backlight on/off
     */
    void setBacklight(bool enable);
    
    /**
     * @brief Set backlight brightness (0-255)
     */
    void setBacklightBrightness(uint8_t brightness);

public:
    // === Advanced Features ===
    
    /**
     * @brief Enable/disable partial display mode
     */
    void setPartialMode(bool enable);
    
    /**
     * @brief Set partial display area
     */
    void setPartialArea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    


public:
    // === Data Transfer Methods ===
    
    /**
     * @brief Write data buffer to display
     * @param data Pointer to data buffer
     * @param length Length of data in bytes
     */
    void writeDataBuffer(const uint8_t* data, size_t length);

public:
    // === Text Rendering ===
    
    /**
     * @brief Draw a character
     */
    void drawChar(uint16_t x, uint16_t y, char c, uint32_t color, uint32_t bg_color);
    
    /**
     * @brief Draw a string (C-style)
     */
    void drawString(uint16_t x, uint16_t y, const char* str, uint32_t color, uint32_t bg_color);
    
    /**
     * @brief Draw a string (string_view)
     */
    void drawString(uint16_t x, uint16_t y, std::string_view str, uint32_t color, uint32_t bg_color);
    
    /**
     * @brief Get string width in pixels
     */
    uint16_t getStringWidth(std::string_view str) const;

public:
    // === Font Control ===
    
    /**
     * @brief Set font layout
     */
    void setFontLayout(FontLayout layout);
    
    /**
     * @brief Get current font layout
     */
    FontLayout getFontLayout() const;

public:
    // === Utility Functions ===
    
    /**
     * @brief Get display width (considering rotation)
     */
    uint16_t getWidth() const;
    
    /**
     * @brief Get display height (considering rotation)
     */
    uint16_t getHeight() const;
    
    /**
     * @brief Check if coordinates are within display bounds
     */
    bool isValidCoordinate(uint16_t x, uint16_t y) const;

private:
    // Implementation details hidden in PIMPL
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace ili9488 
