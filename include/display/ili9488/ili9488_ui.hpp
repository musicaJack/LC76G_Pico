#pragma once

#include <cstdint>
#include <cstddef>

namespace ili9488 {

/**
 * @brief ILI9488 UI Abstract Base Class
 * 
 * Hardware-independent graphics interface inspired by Adafruit GFX.
 * Provides a unified API for graphics operations across different display drivers.
 */
class ILI9488_UI {
public:
    /**
     * @brief Constructor
     * @param width Display width in pixels
     * @param height Display height in pixels
     */
    ILI9488_UI(int16_t width, int16_t height);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~ILI9488_UI();

public:
    // === Pure Virtual Functions (must be implemented by derived classes) ===
    
    /**
     * @brief Write a pixel with RGB565 color
     * @param x X coordinate
     * @param y Y coordinate  
     * @param color RGB565 color value
     */
    virtual void writePixel(uint16_t x, uint16_t y, uint16_t color) = 0;
    
    /**
     * @brief Write a pixel with RGB888 color
     * @param x X coordinate
     * @param y Y coordinate
     * @param color RGB888 color value
     */
    virtual void writePixelRGB24(uint16_t x, uint16_t y, uint32_t color) = 0;

public:
    // === Basic Drawing Functions ===
    
    /**
     * @brief Draw a single pixel
     */
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    
    /**
     * @brief Draw a line from (x0,y0) to (x1,y1)
     */
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    
    /**
     * @brief Draw a fast vertical line
     */
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    
    /**
     * @brief Draw a fast horizontal line
     */
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

public:
    // === Shape Drawing Functions ===
    
    /**
     * @brief Draw a rectangle outline
     */
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    
    /**
     * @brief Draw a filled rectangle
     */
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    
    /**
     * @brief Draw a circle outline
     */
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    
    /**
     * @brief Draw a filled circle
     */
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    
    /**
     * @brief Draw a triangle outline
     */
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    
    /**
     * @brief Draw a filled triangle
     */
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    
    /**
     * @brief Draw a rounded rectangle outline
     */
    void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
    
    /**
     * @brief Draw a filled rounded rectangle
     */
    void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);

public:
    // === Advanced Drawing Functions ===
    
    /**
     * @brief Draw a polygon outline
     */
    void drawPolygon(const int16_t* x_points, const int16_t* y_points, uint8_t count, uint16_t color);
    
    /**
     * @brief Draw a filled polygon
     */
    void fillPolygon(const int16_t* x_points, const int16_t* y_points, uint8_t count, uint16_t color);
    
    /**
     * @brief Draw a bitmap
     */
    void drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* bitmap);
    
    /**
     * @brief Draw an RGB888 bitmap
     */
    void drawBitmapRGB24(int16_t x, int16_t y, int16_t w, int16_t h, const uint32_t* bitmap);

public:
    // === Text Rendering Functions ===
    
    /**
     * @brief Draw a character
     */
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
    
    /**
     * @brief Draw a character with separate X and Y scaling
     */
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);
    
    /**
     * @brief Draw a string
     */
    void drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size);

public:
    // === Screen Control Functions ===
    
    /**
     * @brief Fill the entire screen with a color
     */
    void fillScreen(uint16_t color);
    
    /**
     * @brief Set display rotation
     * @param rotation 0=0°, 1=90°, 2=180°, 3=270°
     */
    void setRotation(uint8_t rotation);
    
    /**
     * @brief Get current rotation
     */
    uint8_t getRotation() const;

public:
    // === Utility Functions ===
    
    /**
     * @brief Get display width (considering rotation)
     */
    int16_t width() const;
    
    /**
     * @brief Get display height (considering rotation)
     */
    int16_t height() const;
    
    /**
     * @brief Check if coordinates are within bounds
     */
    bool isValidCoordinate(int16_t x, int16_t y) const;

protected:
    // === Helper Functions ===
    
    /**
     * @brief Helper function to draw circle quadrants
     */
    void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
    
    /**
     * @brief Helper function to fill circle quadrants
     */
    void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color);
    
    /**
     * @brief Swap two values
     */
    void swap(int16_t& a, int16_t& b);

protected:
    // === Member Variables ===
    
    int16_t _width;     ///< Physical display width
    int16_t _height;    ///< Physical display height
    int16_t WIDTH;      ///< Display width as modified by current rotation
    int16_t HEIGHT;     ///< Display height as modified by current rotation
    uint8_t rotation_;  ///< Current rotation (0-3)
};

// === Inline Implementations ===

inline int16_t ILI9488_UI::width() const {
    return WIDTH;
}

inline int16_t ILI9488_UI::height() const {
    return HEIGHT;
}

inline uint8_t ILI9488_UI::getRotation() const {
    return rotation_;
}

inline bool ILI9488_UI::isValidCoordinate(int16_t x, int16_t y) const {
    return (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT);
}

inline void ILI9488_UI::swap(int16_t& a, int16_t& b) {
    int16_t temp = a;
    a = b;
    b = temp;
}

} // namespace ili9488 