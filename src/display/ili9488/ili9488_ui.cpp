/**
 * @file ili9488_ui.cpp
 * @brief Hardware-independent UI base class implementation
 * @note Provides Adafruit GFX-style API for ILI9488
 */

#include "ili9488_ui.hpp"
#include "ili9488_colors.hpp"

#include <algorithm>
#include <cmath>

namespace ili9488 {

// Constructor
ILI9488_UI::ILI9488_UI(int16_t width, int16_t height) 
    : _width(width), _height(height), WIDTH(width), HEIGHT(height), rotation_(0) {
}

// Destructor  
ILI9488_UI::~ILI9488_UI() = default;

// Drawing primitives with Adafruit GFX compatibility

void ILI9488_UI::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (isValidCoordinate(x, y)) {
        writePixel(static_cast<uint16_t>(x), static_cast<uint16_t>(y), color);
    }
}

void ILI9488_UI::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    
    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }
    
    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }
    
    int16_t dx = x1 - x0;
    int16_t dy = std::abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;
    
    for (; x0 <= x1; x0++) {
        if (steep) {
            drawPixel(y0, x0, color);
        } else {
            drawPixel(x0, y0, color);
        }
        
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void ILI9488_UI::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    drawLine(x, y, x, y + h - 1, color);
}

void ILI9488_UI::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    drawLine(x, y, x + w - 1, y, color);
}

void ILI9488_UI::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x + w - 1, y, h, color);
}

void ILI9488_UI::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t i = x; i < x + w; i++) {
        drawFastVLine(i, y, h, color);
    }
}

void ILI9488_UI::fillScreen(uint16_t color) {
    fillRect(0, 0, WIDTH, HEIGHT, color);
}

void ILI9488_UI::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    drawPixel(x0, y0 + r, color);
    drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color);
    drawPixel(x0 - r, y0, color);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}

void ILI9488_UI::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    drawFastVLine(x0, y0 - r, 2 * r + 1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
}

void ILI9488_UI::fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    int16_t px = x;
    int16_t py = y;
    
    delta++; // Avoid some +1's in the loop
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        // These checks avoid double-drawing certain lines
        if (x < (y + 1)) {
            if (corners & 1) drawFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
            if (corners & 2) drawFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
        }
        if (y != py) {
            if (corners & 1) drawFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
            if (corners & 2) drawFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
            py = y;
        }
        px = x;
    }
}

void ILI9488_UI::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

void ILI9488_UI::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    int16_t a, b, y, last;
    
    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        swap(y0, y1);
        swap(x0, x1);
    }
    if (y1 > y2) {
        swap(y2, y1);
        swap(x2, x1);
    }
    if (y0 > y1) {
        swap(y0, y1);
        swap(x0, x1);
    }
    
    if (y0 == y2) { // Handle awkward all-on-same-line case
        a = b = x0;
        if (x1 < a) a = x1;
        else if (x1 > b) b = x1;
        if (x2 < a) a = x2;
        else if (x2 > b) b = x2;
        drawFastHLine(a, y0, b - a + 1, color);
        return;
    }
    
    int16_t dx01 = x1 - x0;
    int16_t dy01 = y1 - y0;
    int16_t dx02 = x2 - x0;
    int16_t dy02 = y2 - y0;
    int16_t dx12 = x2 - x1;
    int16_t dy12 = y2 - y1;
    int32_t sa = 0;
    int32_t sb = 0;
    
    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2) last = y1;   // Include y1 scanline
    else         last = y1-1; // Skip it
    
    for (y = y0; y <= last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if (a > b) swap(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }
    
    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if (a > b) swap(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }
}

void ILI9488_UI::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if (r > max_radius) r = max_radius;
    // smarter version
    drawFastHLine(x + r, y, w - 2 * r, color); // Top
    drawFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
    drawFastVLine(x, y + r, h - 2 * r, color); // Left
    drawFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
    // draw four corners
    drawCircleHelper(x + r, y + r, r, 1, color);
    drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
    drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
    drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

void ILI9488_UI::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if (r > max_radius) r = max_radius;
    // smarter version
    fillRect(x + r, y, w - 2 * r, h, color);
    // draw four corners
    fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
    fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

void ILI9488_UI::drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (cornername & 0x4) {
            drawPixel(x0 + x, y0 + y, color);
            drawPixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            drawPixel(x0 - y, y0 + x, color);
            drawPixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            drawPixel(x0 - y, y0 - x, color);
            drawPixel(x0 - x, y0 - y, color);
        }
    }
}

void ILI9488_UI::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) {
    drawChar(x, y, c, color, bg, size, size);
}

void ILI9488_UI::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y) {
    if ((x >= WIDTH) || (y >= HEIGHT) ||
        ((x + 6 * size_x - 1) < 0) || ((y + 8 * size_y - 1) < 0))
        return;
    
    // For now, just draw a simple character placeholder
    // This would need to be replaced with actual font rendering
    fillRect(x, y, 6 * size_x, 8 * size_y, bg);
    
    // Simple 5x7 font rendering would go here
    // For demonstration, just draw a filled rectangle
    if (c >= ' ') {
        fillRect(x + size_x, y + size_y, 4 * size_x, 6 * size_y, color);
    }
}

void ILI9488_UI::drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size) {
    int16_t cursor_x = x;
    int16_t cursor_y = y;
    
    while (*str) {
        if (*str == '\n') {
            cursor_y += size * 8;
            cursor_x = x;
        } else if (*str == '\r') {
            cursor_x = x;
        } else {
            drawChar(cursor_x, cursor_y, *str, color, bg, size);
            cursor_x += size * 6;
        }
        str++;
    }
}

void ILI9488_UI::setRotation(uint8_t rotation) {
    rotation_ = rotation & 3;
    switch (rotation_) {
        case 0:
        case 2:
            WIDTH = _width;
            HEIGHT = _height;
            break;
        case 1:
        case 3:
            WIDTH = _height;
            HEIGHT = _width;
            break;
    }
}

void ILI9488_UI::drawPolygon(const int16_t* x_points, const int16_t* y_points, uint8_t count, uint16_t color) {
    if (count < 3) return;
    
    for (uint8_t i = 0; i < count; i++) {
        uint8_t next = (i + 1) % count;
        drawLine(x_points[i], y_points[i], x_points[next], y_points[next], color);
    }
}

void ILI9488_UI::fillPolygon(const int16_t* x_points, const int16_t* y_points, uint8_t count, uint16_t color) {
    // Simple polygon fill using scanline algorithm
    // This is a basic implementation
    if (count < 3) return;
    
    // For now, just draw the outline
    drawPolygon(x_points, y_points, count, color);
}

void ILI9488_UI::drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* bitmap) {
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            drawPixel(x + i, y + j, bitmap[j * w + i]);
        }
    }
}

void ILI9488_UI::drawBitmapRGB24(int16_t x, int16_t y, int16_t w, int16_t h, const uint32_t* bitmap) {
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            writePixelRGB24(x + i, y + j, bitmap[j * w + i]);
        }
    }
}



} // namespace ili9488 