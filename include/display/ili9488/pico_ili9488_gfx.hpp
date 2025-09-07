#pragma once

#include "ili9488_ui.hpp"

namespace pico_ili9488_gfx {

/**
 * @brief Template-based Graphics Engine for ILI9488
 * 
 * High-performance graphics rendering engine using C++ templates.
 * Provides type-safe interface and compile-time optimizations.
 * 
 * @tparam Driver The underlying display driver type
 */
template<typename Driver>
class PicoILI9488GFX : public ili9488::ILI9488_UI {
public:
    /**
     * @brief Constructor
     * @param driver Reference to the display driver
     * @param width Display width in pixels
     * @param height Display height in pixels
     */
    PicoILI9488GFX(Driver& driver, int16_t width, int16_t height);
    
    /**
     * @brief Destructor
     */
    ~PicoILI9488GFX() override;

public:
    // === Implementation of ILI9488_UI Pure Virtual Functions ===
    
    /**
     * @brief Write a pixel with RGB565 color
     * @param x X coordinate (after rotation transformation)
     * @param y Y coordinate (after rotation transformation)
     * @param color RGB565 color value
     */
    void writePixel(uint16_t x, uint16_t y, uint16_t color) override;
    
    /**
     * @brief Write a pixel with RGB888 color
     * @param x X coordinate (after rotation transformation)
     * @param y Y coordinate (after rotation transformation)
     * @param color RGB888 color value
     */
    void writePixelRGB24(uint16_t x, uint16_t y, uint32_t color) override;

public:
    // === Enhanced Drawing Functions ===
    
    /**
     * @brief Fast bitmap drawing with optimized transfer
     */
    void drawBitmapFast(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* bitmap);
    
    /**
     * @brief Fast RGB888 bitmap drawing
     */
    void drawBitmapRGB24Fast(int16_t x, int16_t y, int16_t w, int16_t h, const uint32_t* bitmap);
    
    /**
     * @brief Draw a gradient rectangle
     */
    void drawGradient(int16_t x, int16_t y, int16_t w, int16_t h, 
                      uint32_t color1, uint32_t color2, bool horizontal = true);
    
    /**
     * @brief Draw anti-aliased line
     */
    void drawLineAA(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    
    /**
     * @brief Draw anti-aliased circle
     */
    void drawCircleAA(int16_t x0, int16_t y0, int16_t r, uint16_t color);

public:
    // === Advanced Graphics Effects ===
    
    /**
     * @brief Draw with transparency/alpha blending
     */
    void drawPixelAlpha(int16_t x, int16_t y, uint16_t color, uint8_t alpha);
    
    /**
     * @brief Blend two colors with alpha
     */
    uint16_t blendColors(uint16_t fg, uint16_t bg, uint8_t alpha);
    
    /**
     * @brief Draw a progress bar
     */
    void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, 
                         uint8_t progress, uint16_t fg_color, uint16_t bg_color);
    
    /**
     * @brief Draw a gauge/meter
     */
    void drawGauge(int16_t x, int16_t y, int16_t radius, 
                   float value, float min_val, float max_val,
                   uint16_t color, uint16_t bg_color);

public:
    // === Text Enhancement ===
    
    /**
     * @brief Draw text with shadow effect
     */
    void drawStringWithShadow(int16_t x, int16_t y, const char* str, 
                              uint16_t color, uint16_t shadow_color, 
                              int16_t shadow_offset_x = 1, int16_t shadow_offset_y = 1);
    
    /**
     * @brief Draw outlined text
     */
    void drawStringOutlined(int16_t x, int16_t y, const char* str,
                            uint16_t color, uint16_t outline_color);

public:
    // === Performance Optimized Functions ===
    

    
    /**
     * @brief Fast screen clear using driver's optimized method
     */
    void clearScreenFast(uint16_t color = 0x0000);
    
    /**
     * @brief Optimized rectangle fill using area fill methods
     */
    void fillRectFast(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

public:
    // === Utility Functions ===
    
    /**
     * @brief Get reference to underlying driver
     */
    Driver& getDriver();
    
    /**
     * @brief Get const reference to underlying driver
     */
    const Driver& getDriver() const;
    

    
    /**
     * @brief Check if driver supports partial refresh
     */
    bool supportsPartialRefresh() const;

private:
    Driver& driver_; ///< Reference to the underlying display driver
};

// === Template Method Implementations ===

template<typename Driver>
inline Driver& PicoILI9488GFX<Driver>::getDriver() {
    return driver_;
}

template<typename Driver>
inline const Driver& PicoILI9488GFX<Driver>::getDriver() const {
    return driver_;
}

} // namespace pico_ili9488_gfx

// Include template implementation
#include "pico_ili9488_gfx.inl" 
