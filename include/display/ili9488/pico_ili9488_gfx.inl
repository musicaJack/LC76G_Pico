// Template implementation file for PicoILI9488GFX
// This file should be included at the end of pico_ili9488_gfx.hpp

#include <cmath>

namespace pico_ili9488_gfx {

template<typename Driver>
PicoILI9488GFX<Driver>::PicoILI9488GFX(Driver& driver, int16_t width, int16_t height)
    : ili9488::ILI9488_UI(width, height), driver_(driver) {
    // Constructor implementation
}

template<typename Driver>
PicoILI9488GFX<Driver>::~PicoILI9488GFX() {
    // Destructor implementation
}

template<typename Driver>
void PicoILI9488GFX<Driver>::writePixel(uint16_t x, uint16_t y, uint16_t color) {
    // Delegate to the underlying driver
    driver_.drawPixel(x, y, color);
}

template<typename Driver>
void PicoILI9488GFX<Driver>::writePixelRGB24(uint16_t x, uint16_t y, uint32_t color) {
    // Delegate to the underlying driver
    driver_.drawPixelRGB24(x, y, color);
}

template<typename Driver>
void PicoILI9488GFX<Driver>::drawBitmapFast(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* bitmap) {
    // Simple fallback to standard bitmap drawing
    ili9488::ILI9488_UI::drawBitmap(x, y, w, h, bitmap);
}

template<typename Driver>
void PicoILI9488GFX<Driver>::drawBitmapRGB24Fast(int16_t x, int16_t y, int16_t w, int16_t h, const uint32_t* bitmap) {
    // Simple fallback to standard RGB24 bitmap drawing
    ili9488::ILI9488_UI::drawBitmapRGB24(x, y, w, h, bitmap);
}

template<typename Driver>
void PicoILI9488GFX<Driver>::clearScreenFast(uint16_t color) {
    // Use base class method
    ili9488::ILI9488_UI::fillScreen(color);
}

template<typename Driver>
void PicoILI9488GFX<Driver>::fillRectFast(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    // Use base class method
    ili9488::ILI9488_UI::fillRect(x, y, w, h, color);
}



template<typename Driver>
bool PicoILI9488GFX<Driver>::supportsPartialRefresh() const {
    // For now, return false - can be enhanced later with proper feature detection
    return false;
}



template<typename Driver>
uint16_t PicoILI9488GFX<Driver>::blendColors(uint16_t fg, uint16_t bg, uint8_t alpha) {
    // Simple alpha blending for RGB565
    if (alpha == 255) return fg;
    if (alpha == 0) return bg;
    
    // Extract RGB components from RGB565
    uint8_t fg_r = (fg >> 11) & 0x1F;
    uint8_t fg_g = (fg >> 5) & 0x3F;
    uint8_t fg_b = fg & 0x1F;
    
    uint8_t bg_r = (bg >> 11) & 0x1F;
    uint8_t bg_g = (bg >> 5) & 0x3F;
    uint8_t bg_b = bg & 0x1F;
    
    // Blend components
    uint8_t r = (fg_r * alpha + bg_r * (255 - alpha)) / 255;
    uint8_t g = (fg_g * alpha + bg_g * (255 - alpha)) / 255;
    uint8_t b = (fg_b * alpha + bg_b * (255 - alpha)) / 255;
    
    // Recombine to RGB565
    return (r << 11) | (g << 5) | b;
}

template<typename Driver>
void PicoILI9488GFX<Driver>::drawPixelAlpha(int16_t x, int16_t y, uint16_t color, uint8_t alpha) {
    if (alpha == 255) {
        writePixel(x, y, color);
        return;
    }
    
    // For alpha blending, we would need to read the current pixel value
    // This is a simplified implementation
    writePixel(x, y, color);
}

template<typename Driver>
void PicoILI9488GFX<Driver>::drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, 
                                              uint8_t progress, uint16_t fg_color, uint16_t bg_color) {
    // Draw background
    ili9488::ILI9488_UI::fillRect(x, y, w, h, bg_color);
    
    // Draw progress
    int16_t progress_width = (w * progress) / 100;
    if (progress_width > 0) {
        ili9488::ILI9488_UI::fillRect(x, y, progress_width, h, fg_color);
    }
}

template<typename Driver>
void PicoILI9488GFX<Driver>::drawGradient(int16_t x, int16_t y, int16_t w, int16_t h, 
                                           uint32_t color1, uint32_t color2, bool horizontal) {
    // Simple gradient implementation
    for (int16_t i = 0; i < (horizontal ? w : h); i++) {
        uint8_t alpha = (255 * i) / (horizontal ? w : h);
        uint16_t blended = blendColors(static_cast<uint16_t>(color2), static_cast<uint16_t>(color1), alpha);
        
        if (horizontal) {
            ili9488::ILI9488_UI::drawFastVLine(x + i, y, h, blended);
        } else {
            ili9488::ILI9488_UI::drawFastHLine(x, y + i, w, blended);
        }
    }
}

template<typename Driver>
void PicoILI9488GFX<Driver>::drawLineAA(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    // Simple fallback to regular line
    ili9488::ILI9488_UI::drawLine(x0, y0, x1, y1, color);
}

template<typename Driver>
void PicoILI9488GFX<Driver>::drawCircleAA(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    // Simple fallback to regular circle
    ili9488::ILI9488_UI::drawCircle(x0, y0, r, color);
}

template<typename Driver>
void PicoILI9488GFX<Driver>::drawStringWithShadow(int16_t x, int16_t y, const char* str, 
                                                   uint16_t color, uint16_t shadow_color, 
                                                   int16_t shadow_offset_x, int16_t shadow_offset_y) {
    // Draw shadow first
    ili9488::ILI9488_UI::drawString(x + shadow_offset_x, y + shadow_offset_y, str, shadow_color, 0, 1);
    // Draw main text
    ili9488::ILI9488_UI::drawString(x, y, str, color, 0, 1);
}

template<typename Driver>
void PicoILI9488GFX<Driver>::drawStringOutlined(int16_t x, int16_t y, const char* str,
                                                 uint16_t color, uint16_t outline_color) {
    // Draw outline in 8 directions
    for (int8_t dx = -1; dx <= 1; dx++) {
        for (int8_t dy = -1; dy <= 1; dy++) {
            if (dx != 0 || dy != 0) {
                ili9488::ILI9488_UI::drawString(x + dx, y + dy, str, outline_color, 0, 1);
            }
        }
    }
    // Draw main text
    ili9488::ILI9488_UI::drawString(x, y, str, color, 0, 1);
}

template<typename Driver>
void PicoILI9488GFX<Driver>::drawGauge(int16_t x, int16_t y, int16_t radius, 
                                        float value, float min_val, float max_val,
                                        uint16_t color, uint16_t bg_color) {
    // Draw gauge background
    ili9488::ILI9488_UI::drawCircle(x, y, radius, bg_color);
    
    // Calculate angle for value
    float angle = 3.14159f * (value - min_val) / (max_val - min_val);
    int16_t end_x = x + static_cast<int16_t>(radius * 0.8f * cos(angle));
    int16_t end_y = y + static_cast<int16_t>(radius * 0.8f * sin(angle));
    
    // Draw gauge needle
    ili9488::ILI9488_UI::drawLine(x, y, end_x, end_y, color);
}

} // namespace pico_ili9488_gfx 