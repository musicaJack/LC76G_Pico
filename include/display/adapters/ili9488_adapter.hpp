#pragma once

#include "display_driver.hpp"
#include "ili9488_driver.hpp"

namespace game2048 {

// ILI9488显示驱动适配器
class ILI9488Adapter : public DisplayDriver {
public:
    ILI9488Adapter(const DisplayConfig& config);
    virtual ~ILI9488Adapter();
    
    // 初始化
    bool init() override;
    void deinit() override;
    
    // 显示控制
    void clear() override;
    void clear(const Color& color) override;
    void display() override;
    
    // 基本绘图功能
    void drawPixel(int16_t x, int16_t y, const Color& color) override;
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const Color& color) override;
    void drawRect(int16_t x, int16_t y, int16_t width, int16_t height, const Color& color) override;
    void fillRect(int16_t x, int16_t y, int16_t width, int16_t height, const Color& color) override;
    void drawCircle(int16_t x, int16_t y, int16_t radius, const Color& color) override;
    void fillCircle(int16_t x, int16_t y, int16_t radius, const Color& color) override;
    
    // 文本绘制
    void drawChar(int16_t x, int16_t y, char c, const Color& color, uint8_t size = 1) override;
    void drawString(int16_t x, int16_t y, const std::string& text, const Color& color, uint8_t size = 1) override;
    void drawString(int16_t x, int16_t y, const std::string& text, const Color& color, const Color& bgColor, uint8_t size = 1) override;
    
    // Game2048优化字体绘制
    void drawStringGame2048(int16_t x, int16_t y, const std::string& text, const Color& color, uint8_t size = 1);
    void drawStringGame2048(int16_t x, int16_t y, const std::string& text, const Color& color, const Color& bgColor, uint8_t size = 1);
    
    // 显示属性
    uint16_t getWidth() const override;
    uint16_t getHeight() const override;
    void setRotation(uint8_t rotation) override;
    uint8_t getRotation() const override;
    
    // 颜色转换
    uint16_t colorToNative(const Color& color) override;
    Color nativeToColor(uint16_t native_color) override;
    
    // 特殊功能
    void setBrightness(uint8_t brightness) override;
    uint8_t getBrightness() const override;
    
    // 缓冲区操作
    void* getBuffer() override;
    size_t getBufferSize() const override;
    
private:
    DisplayConfig config_;
    ili9488::ILI9488Driver* driver_;
    
    // 颜色转换（RGB888到RGB565）
    uint16_t rgb888ToRgb565(const Color& color) const;
    
    // 坐标转换（处理旋转）
    void transformCoordinates(int16_t& x, int16_t& y) const;
};

} // namespace game2048
