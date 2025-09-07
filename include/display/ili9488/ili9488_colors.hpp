#pragma once

#include <cstdint>

namespace ili9488_colors {

// === RGB565 Color Constants ===
namespace rgb565 {
    constexpr uint16_t BLACK   = 0x0000;   // 0,   0,   0
    constexpr uint16_t NAVY    = 0x000F;   // 0,   0, 128
    constexpr uint16_t DARKGREEN = 0x03E0; // 0, 128,   0
    constexpr uint16_t DARKCYAN = 0x03EF;  // 0, 128, 128
    constexpr uint16_t MAROON  = 0x7800;   // 128, 0,   0
    constexpr uint16_t PURPLE  = 0x780F;   // 128, 0, 128
    constexpr uint16_t OLIVE   = 0x7BE0;   // 128,128,   0
    constexpr uint16_t LIGHTGREY = 0xC618; // 192,192, 192
    constexpr uint16_t DARKGREY = 0x7BEF;  // 128,128, 128
    constexpr uint16_t BLUE    = 0x001F;   // 0,   0, 255
    constexpr uint16_t GREEN   = 0x07E0;   // 0, 255,   0
    constexpr uint16_t CYAN    = 0x07FF;   // 0, 255, 255
    constexpr uint16_t RED     = 0xF800;   // 255, 0,   0
    constexpr uint16_t MAGENTA = 0xF81F;   // 255, 0, 255
    constexpr uint16_t YELLOW  = 0xFFE0;   // 255,255,   0
    constexpr uint16_t WHITE   = 0xFFFF;   // 255,255, 255
    constexpr uint16_t ORANGE  = 0xFD20;   // 255,165,   0
    constexpr uint16_t GREENYELLOW = 0xAFE5; // 173,255,  47
    constexpr uint16_t PINK    = 0xF81F;   // 255,192, 203
    
    // Missing color constants
    constexpr uint16_t DARKBLUE = 0x0010;  // 0,0,128
    constexpr uint16_t DARKGRAY = DARKGREY; // Alias for DARKGREY
}

// === RGB666 Color Constants (ILI9488 Native Format) ===
namespace rgb666 {
    constexpr uint32_t BLACK   = 0x000000;
    constexpr uint32_t NAVY    = 0x000080;
    constexpr uint32_t DARKGREEN = 0x008000;
    constexpr uint32_t DARKCYAN = 0x008080;
    constexpr uint32_t MAROON  = 0x800000;
    constexpr uint32_t PURPLE  = 0x800080;
    constexpr uint32_t OLIVE   = 0x808000;
    constexpr uint32_t LIGHTGREY = 0xC0C0C0;
    constexpr uint32_t DARKGREY = 0x808080;
    constexpr uint32_t BLUE    = 0x0000FC;   // ILI9488 specific
    constexpr uint32_t GREEN   = 0x00FC00;   // ILI9488 specific
    constexpr uint32_t CYAN    = 0x00FCFC;   // ILI9488 specific
    constexpr uint32_t RED     = 0xFC0000;   // ILI9488 specific
    constexpr uint32_t MAGENTA = 0xFC00FC;   // ILI9488 specific
    constexpr uint32_t YELLOW  = 0xFCFC00;   // ILI9488 specific
    constexpr uint32_t WHITE   = 0xFCFCFC;   // ILI9488 specific
    constexpr uint32_t ORANGE  = 0xFC8000;
    constexpr uint32_t GREENYELLOW = 0x80FC00;
    constexpr uint32_t PINK    = 0xFCC0C0;
    
    // === 新增贪吃蛇游戏专用颜色 ===
    // 荧光绿色系 (适合蛇身)
    constexpr uint32_t NEON_GREEN = 0x40FC40; // 霓虹绿
    constexpr uint32_t BRIGHT_GREEN = 0x00FC80; // 亮绿色
    constexpr uint32_t DARK_GREEN = 0x2C390C; // 自定义深绿色

    
    // 荧光粉色系 (适合蛇头)
    constexpr uint32_t PINK_RED = 0xFC3498;    // 自定义粉红色
    constexpr uint32_t NEON_PINK = 0xFC40FC;   // 霓虹粉
    constexpr uint32_t HOT_PINK = 0xFC4080;    // 热粉色
    constexpr uint32_t BRIGHT_MAGENTA = 0xFC00C0; // 亮洋红
    constexpr uint32_t ELECTRIC_PINK = 0xFC80FC;  // 电光粉
    
    // 其他游戏常用亮色
    constexpr uint32_t NEON_BLUE = 0x4040FC;   // 霓虹蓝
    constexpr uint32_t NEON_YELLOW = 0xFCFC40; // 霓虹黄
    constexpr uint32_t ELECTRIC_CYAN = 0x40FCFC; // 电光青
}

// === RGB888 Color Constants (24-bit True Color) ===
namespace rgb888 {
    constexpr uint32_t BLACK   = 0x000000;
    constexpr uint32_t NAVY    = 0x000080;
    constexpr uint32_t DARKGREEN = 0x008000;
    constexpr uint32_t DARKCYAN = 0x008080;
    constexpr uint32_t MAROON  = 0x800000;
    constexpr uint32_t PURPLE  = 0x800080;
    constexpr uint32_t OLIVE   = 0x808000;
    constexpr uint32_t LIGHTGREY = 0xC0C0C0;
    constexpr uint32_t DARKGREY = 0x808080;
    constexpr uint32_t BLUE    = 0x0000FF;
    constexpr uint32_t GREEN   = 0x00FF00;
    constexpr uint32_t CYAN    = 0x00FFFF;
    constexpr uint32_t RED     = 0xFF0000;
    constexpr uint32_t MAGENTA = 0xFF00FF;
    constexpr uint32_t YELLOW  = 0xFFFF00;
    constexpr uint32_t WHITE   = 0xFFFFFF;
    constexpr uint32_t ORANGE  = 0xFFA500;
    constexpr uint32_t GREENYELLOW = 0xADFF2F;
    constexpr uint32_t PINK    = 0xFFC0CB;
    
    // Missing color constants  
    constexpr uint32_t DARKBLUE = 0x000080;  // Dark blue
    constexpr uint32_t DARKGRAY = DARKGREY;  // Alias for DARKGREY
}

// === Color Conversion Functions ===

/**
 * @brief Convert RGB888 to RGB565
 * @param rgb888 24-bit RGB color (0xRRGGBB)
 * @return 16-bit RGB565 color
 */
constexpr uint16_t rgb888_to_rgb565(uint32_t rgb888) {
    const uint8_t r = (rgb888 >> 16) & 0xFF;
    const uint8_t g = (rgb888 >> 8) & 0xFF;
    const uint8_t b = rgb888 & 0xFF;
    
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief Convert RGB565 to RGB888
 * @param rgb565 16-bit RGB565 color
 * @return 24-bit RGB888 color
 */
constexpr uint32_t rgb565_to_rgb888(uint16_t rgb565) {
    const uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;
    const uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;
    const uint8_t b = (rgb565 & 0x1F) << 3;
    
    return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

/**
 * @brief Convert RGB888 to RGB666 (ILI9488 native)
 * @param rgb888 24-bit RGB color
 * @return 18-bit RGB666 color (stored in 32-bit)
 */
constexpr uint32_t rgb888_to_rgb666(uint32_t rgb888) {
    const uint8_t r = (rgb888 >> 16) & 0xFF;
    const uint8_t g = (rgb888 >> 8) & 0xFF;
    const uint8_t b = rgb888 & 0xFF;
    
    // Convert 8-bit to 6-bit
    const uint8_t r6 = r >> 2;
    const uint8_t g6 = g >> 2;
    const uint8_t b6 = b >> 2;
    
    return (uint32_t(r6) << 12) | (uint32_t(g6) << 6) | uint32_t(b6);
}

/**
 * @brief Convert RGB666 to RGB888
 * @param rgb666 18-bit RGB666 color
 * @return 24-bit RGB888 color
 */
constexpr uint32_t rgb666_to_rgb888(uint32_t rgb666) {
    const uint8_t r6 = (rgb666 >> 12) & 0x3F;
    const uint8_t g6 = (rgb666 >> 6) & 0x3F;
    const uint8_t b6 = rgb666 & 0x3F;
    
    // Convert 6-bit to 8-bit
    const uint8_t r = (r6 << 2) | (r6 >> 4);
    const uint8_t g = (g6 << 2) | (g6 >> 4);
    const uint8_t b = (b6 << 2) | (b6 >> 4);
    
    return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

/**
 * @brief Convert RGB565 to RGB666
 * @param rgb565 16-bit RGB565 color
 * @return 18-bit RGB666 color
 */
constexpr uint32_t rgb565_to_rgb666(uint16_t rgb565) {
    return rgb888_to_rgb666(rgb565_to_rgb888(rgb565));
}

/**
 * @brief Convert RGB666 to RGB565
 * @param rgb666 18-bit RGB666 color
 * @return 16-bit RGB565 color
 */
constexpr uint16_t rgb666_to_rgb565(uint32_t rgb666) {
    return rgb888_to_rgb565(rgb666_to_rgb888(rgb666));
}

/**
 * @brief Create RGB565 color from individual R, G, B components
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return RGB565 color
 */
constexpr uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief Create RGB888 color from individual R, G, B components
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return RGB888 color
 */
constexpr uint32_t color888(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

/**
 * @brief Create RGB666 color from individual R, G, B components
 * @param r Red component (0-63)
 * @param g Green component (0-63)
 * @param b Blue component (0-63)
 * @return RGB666 color
 */
constexpr uint32_t color666(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t(r & 0x3F) << 12) | (uint32_t(g & 0x3F) << 6) | uint32_t(b & 0x3F);
}

/**
 * @brief Extract red component from RGB565
 * @param rgb565 RGB565 color
 * @return Red component (0-31)
 */
constexpr uint8_t red565(uint16_t rgb565) {
    return (rgb565 >> 11) & 0x1F;
}

/**
 * @brief Extract green component from RGB565
 * @param rgb565 RGB565 color
 * @return Green component (0-63)
 */
constexpr uint8_t green565(uint16_t rgb565) {
    return (rgb565 >> 5) & 0x3F;
}

/**
 * @brief Extract blue component from RGB565
 * @param rgb565 RGB565 color
 * @return Blue component (0-31)
 */
constexpr uint8_t blue565(uint16_t rgb565) {
    return rgb565 & 0x1F;
}

/**
 * @brief Extract red component from RGB888
 * @param rgb888 RGB888 color
 * @return Red component (0-255)
 */
constexpr uint8_t red888(uint32_t rgb888) {
    return (rgb888 >> 16) & 0xFF;
}

/**
 * @brief Extract green component from RGB888
 * @param rgb888 RGB888 color
 * @return Green component (0-255)
 */
constexpr uint8_t green888(uint32_t rgb888) {
    return (rgb888 >> 8) & 0xFF;
}

/**
 * @brief Extract blue component from RGB888
 * @param rgb888 RGB888 color
 * @return Blue component (0-255)
 */
constexpr uint8_t blue888(uint32_t rgb888) {
    return rgb888 & 0xFF;
}

// === Convenience Functions for Namespaces ===

namespace rgb565 {
    // Convenience functions
    constexpr uint16_t from_rgb888(uint8_t r, uint8_t g, uint8_t b) {
        return color565(r, g, b);
    }
} // namespace rgb565

namespace rgb888 {
    // Convenience functions
    constexpr uint32_t from_rgb565(uint16_t rgb565) {
        return rgb565_to_rgb888(rgb565);
    }
    
    constexpr uint32_t from_rgb888(uint8_t r, uint8_t g, uint8_t b) {
        return color888(r, g, b);
    }
} // namespace rgb888

} // namespace ili9488_colors 