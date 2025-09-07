#pragma once

#include <cstdint>
#include <string>

namespace game2048 {

// 颜色结构体 - 统一RGB格式
struct Color {
    uint8_t r, g, b;
    
    Color() : r(0), g(0), b(0) {}
    Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
    
    // 预定义颜色
    static const Color BLACK;
    static const Color WHITE;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color GRAY;
};

// 点结构体
struct Point {
    int16_t x, y;
    
    Point() : x(0), y(0) {}
    Point(int16_t x_pos, int16_t y_pos) : x(x_pos), y(y_pos) {}
};

// 矩形结构体
struct Rect {
    int16_t x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(int16_t x_pos, int16_t y_pos, int16_t w, int16_t h) 
        : x(x_pos), y(y_pos), width(w), height(h) {}
};

// 显示驱动抽象基类
class DisplayDriver {
public:
    virtual ~DisplayDriver() = default;
    
    // 初始化
    virtual bool init() = 0;
    virtual void deinit() = 0;
    
    // 显示控制
    virtual void clear() = 0;
    virtual void clear(const Color& color) = 0;
    virtual void display() = 0;
    
    // 基本绘图功能
    virtual void drawPixel(int16_t x, int16_t y, const Color& color) = 0;
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const Color& color) = 0;
    virtual void drawRect(int16_t x, int16_t y, int16_t width, int16_t height, const Color& color) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t width, int16_t height, const Color& color) = 0;
    virtual void drawCircle(int16_t x, int16_t y, int16_t radius, const Color& color) = 0;
    virtual void fillCircle(int16_t x, int16_t y, int16_t radius, const Color& color) = 0;
    
    // 文本绘制
    virtual void drawChar(int16_t x, int16_t y, char c, const Color& color, uint8_t size = 1) = 0;
    virtual void drawString(int16_t x, int16_t y, const std::string& text, const Color& color, uint8_t size = 1) = 0;
    virtual void drawString(int16_t x, int16_t y, const std::string& text, const Color& color, const Color& bgColor, uint8_t size = 1) = 0;
    
    // 显示属性
    virtual uint16_t getWidth() const = 0;
    virtual uint16_t getHeight() const = 0;
    virtual void setRotation(uint8_t rotation) = 0;
    virtual uint8_t getRotation() const = 0;
    
    // 颜色转换（用于不同显示器的颜色映射）
    virtual uint16_t colorToNative(const Color& color) = 0;
    virtual Color nativeToColor(uint16_t native_color) = 0;
    
    // 特殊功能（可选）
    virtual void setBrightness(uint8_t brightness) = 0;
    virtual uint8_t getBrightness() const = 0;
    
    // 缓冲区操作
    virtual void* getBuffer() = 0;
    virtual size_t getBufferSize() const = 0;
};

// 预定义颜色实现
inline const Color Color::BLACK = Color(0, 0, 0);
inline const Color Color::WHITE = Color(255, 255, 255);
inline const Color Color::RED = Color(255, 0, 0);
inline const Color Color::GREEN = Color(0, 255, 0);
inline const Color Color::BLUE = Color(0, 0, 255);
inline const Color Color::YELLOW = Color(255, 255, 0);
inline const Color Color::GRAY = Color(128, 128, 128);

} // namespace game2048
