#include "ili9488_adapter.hpp"
#include "ili9488_font.hpp"
#include <cstring>
#include <cmath>

namespace game2048 {

ILI9488Adapter::ILI9488Adapter(const DisplayConfig& config) 
    : config_(config), driver_(nullptr) {
}

ILI9488Adapter::~ILI9488Adapter() {
    deinit();
}

bool ILI9488Adapter::init() {
    // 创建ILI9488驱动实例
    driver_ = new ili9488::ILI9488Driver(
        spi0,  // 使用SPI0
        config_.pins.dc, 
        config_.pins.rst, 
        config_.pins.cs, 
        config_.pins.sclk, 
        config_.pins.mosi,
        config_.pins.bl,
        config_.hardware.spi_frequency
    );
    if (!driver_) {
        return false;
    }
    
    // 初始化驱动
    if (!driver_->initialize()) {
        delete driver_;
        driver_ = nullptr;
        return false;
    }
    
    // 设置旋转
    setRotation(config_.rotation);
    
    // 设置亮度
    setBrightness(config_.brightness);
    
    return true;
}

void ILI9488Adapter::deinit() {
    if (driver_) {
        delete driver_;
        driver_ = nullptr;
    }
}

void ILI9488Adapter::clear() {
    if (driver_) {
        driver_->clear();
    }
}

void ILI9488Adapter::clear(const Color& color) {
    if (driver_) {
        uint16_t rgb565 = rgb888ToRgb565(color);
        driver_->clear();
        // 填充整个屏幕
        for (int16_t y = 0; y < getHeight(); y++) {
            for (int16_t x = 0; x < getWidth(); x++) {
                driver_->drawPixel(x, y, rgb565);
            }
        }
    }
}

void ILI9488Adapter::display() {
    // ILI9488驱动不需要显式的display()调用，像素直接写入
    // 这里可以用于刷新或同步操作
}

void ILI9488Adapter::drawPixel(int16_t x, int16_t y, const Color& color) {
    if (!driver_) return;
    
    transformCoordinates(x, y);
    if (x >= 0 && x < getWidth() && y >= 0 && y < getHeight()) {
        uint16_t rgb565 = rgb888ToRgb565(color);
        driver_->drawPixel(x, y, rgb565);
    }
}

void ILI9488Adapter::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const Color& color) {
    if (!driver_) return;
    
    uint16_t rgb565 = rgb888ToRgb565(color);
    
    // 简单的Bresenham直线算法
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx - dy;
    
    while (true) {
        drawPixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void ILI9488Adapter::drawRect(int16_t x, int16_t y, int16_t width, int16_t height, const Color& color) {
    if (!driver_) return;
    
    // 绘制矩形的四条边
    drawLine(x, y, x + width - 1, y, color);  // 上边
    drawLine(x, y + height - 1, x + width - 1, y + height - 1, color);  // 下边
    drawLine(x, y, x, y + height - 1, color);  // 左边
    drawLine(x + width - 1, y, x + width - 1, y + height - 1, color);  // 右边
}

void ILI9488Adapter::fillRect(int16_t x, int16_t y, int16_t width, int16_t height, const Color& color) {
    if (!driver_) return;
    
    // 使用底层驱动的高效方法
    uint32_t rgb666 = (color.r << 16) | (color.g << 8) | color.b;
    driver_->fillAreaRGB666(x, y, x + width - 1, y + height - 1, rgb666);
}

void ILI9488Adapter::drawCircle(int16_t x, int16_t y, int16_t radius, const Color& color) {
    if (!driver_) return;
    
    // 简单的圆形绘制算法
    for (int16_t i = -radius; i <= radius; i++) {
        for (int16_t j = -radius; j <= radius; j++) {
            if (i * i + j * j <= radius * radius) {
                drawPixel(x + i, y + j, color);
            }
        }
    }
}

void ILI9488Adapter::fillCircle(int16_t x, int16_t y, int16_t radius, const Color& color) {
    if (!driver_) return;
    
    // 填充圆形
    for (int16_t i = -radius; i <= radius; i++) {
        for (int16_t j = -radius; j <= radius; j++) {
            if (i * i + j * j <= radius * radius) {
                drawPixel(x + i, y + j, color);
            }
        }
    }
}

void ILI9488Adapter::drawChar(int16_t x, int16_t y, char c, const Color& color, uint8_t size) {
    if (!driver_) return;
    
    // 简单的字符绘制（8x8字体）
    uint16_t rgb565 = rgb888ToRgb565(color);
    
    // 这里应该实现字体渲染，暂时用简单的方块代替
    for (int16_t i = 0; i < 8 * size; i++) {
        for (int16_t j = 0; j < 8 * size; j++) {
            drawPixel(x + j, y + i, color);
        }
    }
}

void ILI9488Adapter::drawString(int16_t x, int16_t y, const std::string& text, const Color& color, uint8_t size) {
    if (!driver_) return;
    
    // 转换颜色格式
    uint32_t rgb888_color = (color.r << 16) | (color.g << 8) | color.b;
    uint32_t bg_color = 0x000000;  // 默认黑色背景
    
    // 调用底层驱动的drawString方法
    driver_->drawString(x, y, text, rgb888_color, bg_color);
}

void ILI9488Adapter::drawString(int16_t x, int16_t y, const std::string& text, const Color& color, const Color& bgColor, uint8_t size) {
    if (!driver_) return;
    
    // 转换颜色格式
    uint32_t rgb888_color = (color.r << 16) | (color.g << 8) | color.b;
    uint32_t bg_color = (bgColor.r << 16) | (bgColor.g << 8) | bgColor.b;
    
    // 调用底层驱动的drawString方法
    driver_->drawString(x, y, text, rgb888_color, bg_color);
}

uint16_t ILI9488Adapter::getWidth() const {
    return ili9488::ILI9488Driver::LCD_WIDTH;
}

uint16_t ILI9488Adapter::getHeight() const {
    return ili9488::ILI9488Driver::LCD_HEIGHT;
}

void ILI9488Adapter::setRotation(uint8_t rotation) {
    config_.rotation = rotation;
    if (driver_) {
        // ILI9488驱动可能需要设置旋转
        // 这里暂时不实现，因为原始驱动可能没有这个方法
    }
}

uint8_t ILI9488Adapter::getRotation() const {
    return config_.rotation;
}

uint16_t ILI9488Adapter::colorToNative(const Color& color) {
    return rgb888ToRgb565(color);
}

Color ILI9488Adapter::nativeToColor(uint16_t native_color) {
    // 将RGB565转换回RGB888
    uint8_t r = ((native_color >> 11) & 0x1F) << 3;
    uint8_t g = ((native_color >> 5) & 0x3F) << 2;
    uint8_t b = (native_color & 0x1F) << 3;
    return Color(r, g, b);
}

void ILI9488Adapter::setBrightness(uint8_t brightness) {
    config_.brightness = brightness;
    // ILI9488可能通过背光控制亮度
    // 这里暂时不实现
}

uint8_t ILI9488Adapter::getBrightness() const {
    return config_.brightness;
}

void* ILI9488Adapter::getBuffer() {
    // ILI9488驱动可能不暴露直接缓冲区
    return nullptr;
}

size_t ILI9488Adapter::getBufferSize() const {
    // ILI9488缓冲区大小
    return getWidth() * getHeight() * 2;  // RGB565 = 2 bytes per pixel
}

uint16_t ILI9488Adapter::rgb888ToRgb565(const Color& color) const {
    // 将RGB888转换为RGB565
    uint8_t r = (color.r >> 3) & 0x1F;  // 5 bits
    uint8_t g = (color.g >> 2) & 0x3F;  // 6 bits
    uint8_t b = (color.b >> 3) & 0x1F;  // 5 bits
    
    return (r << 11) | (g << 5) | b;
}

void ILI9488Adapter::transformCoordinates(int16_t& x, int16_t& y) const {
    // 根据旋转角度转换坐标
    uint16_t width = getWidth();
    uint16_t height = getHeight();
    
    switch (config_.rotation) {
        case 1: // 90度
            {
                int16_t temp = x;
                x = y;
                y = width - 1 - temp;
            }
            break;
        case 2: // 180度
            x = width - 1 - x;
            y = height - 1 - y;
            break;
        case 3: // 270度
            {
                int16_t temp = x;
                x = height - 1 - y;
                y = temp;
            }
            break;
        default: // 0度，不需要转换
            break;
    }
}

// Game2048优化字体绘制方法
void ILI9488Adapter::drawStringGame2048(int16_t x, int16_t y, const std::string& text, const Color& color, uint8_t size) {
    if (!driver_) return;
    
    uint16_t rgb565 = rgb888ToRgb565(color);
    int16_t currentX = x;
    
    for (char c : text) {
        const uint8_t* fontData = font::get_game2048_char_data(c);
        if (!fontData) continue;
        
        for (int row = 0; row < font::GAME2048_FONT_HEIGHT; ++row) {
            uint8_t rowData = fontData[row];
            for (int col = 0; col < font::GAME2048_FONT_WIDTH; ++col) {
                if (rowData & (0x80 >> col)) {
                    for (int sx = 0; sx < size; ++sx) {
                        for (int sy = 0; sy < size; ++sy) {
                            int16_t px = currentX + col * size + sx;
                            int16_t py = y + row * size + sy;
                            transformCoordinates(px, py);
                            if (px >= 0 && px < getWidth() && py >= 0 && py < getHeight()) {
                                driver_->drawPixel(px, py, rgb565);
                            }
                        }
                    }
                }
            }
        }
        currentX += font::GAME2048_FONT_WIDTH * size;
    }
}

void ILI9488Adapter::drawStringGame2048(int16_t x, int16_t y, const std::string& text, const Color& color, const Color& bgColor, uint8_t size) {
    if (!driver_) return;
    
    uint16_t textRgb565 = rgb888ToRgb565(color);
    uint16_t bgRgb565 = rgb888ToRgb565(bgColor);
    int16_t currentX = x;
    
    for (char c : text) {
        const uint8_t* fontData = font::get_game2048_char_data(c);
        if (!fontData) continue;
        
        for (int row = 0; row < font::GAME2048_FONT_HEIGHT; ++row) {
            uint8_t rowData = fontData[row];
            for (int col = 0; col < font::GAME2048_FONT_WIDTH; ++col) {
                uint16_t pixelColor = (rowData & (0x80 >> col)) ? textRgb565 : bgRgb565;
                for (int sx = 0; sx < size; ++sx) {
                    for (int sy = 0; sy < size; ++sy) {
                        int16_t px = currentX + col * size + sx;
                        int16_t py = y + row * size + sy;
                        transformCoordinates(px, py);
                        if (px >= 0 && px < getWidth() && py >= 0 && py < getHeight()) {
                            driver_->drawPixel(px, py, pixelColor);
                        }
                    }
                }
            }
        }
        currentX += font::GAME2048_FONT_WIDTH * size;
    }
}

} // namespace game2048
