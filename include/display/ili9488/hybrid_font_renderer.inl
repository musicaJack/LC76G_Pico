#pragma once

#include <cstdio>

namespace hybrid_font {

// ============================================================================
// FontRenderer 模板实现
// ============================================================================

template<typename DisplayDriver>
FontRenderer<DisplayDriver>::FontRenderer(std::shared_ptr<IFontDataSource> font_source) 
    : font_source_(font_source) {
}

template<typename DisplayDriver>
void FontRenderer<DisplayDriver>::set_font_source(std::shared_ptr<IFontDataSource> font_source) {
    font_source_ = font_source;
}

template<typename DisplayDriver>
std::shared_ptr<IFontDataSource> FontRenderer<DisplayDriver>::get_font_source() const {
    return font_source_;
}

template<typename DisplayDriver>
void FontRenderer<DisplayDriver>::draw_char(DisplayDriver& display, int x, int y, 
                                           uint32_t char_code, bool color) {
    if (!font_source_ || !font_source_->is_valid()) {
        return;
    }
    
    std::vector<uint8_t> bitmap = font_source_->get_char_bitmap(char_code);
    if (bitmap.empty()) {
        return;
    }
    
    // 根据字符类型选择不同的绘制方法
    // ASCII字符使用8x16绘制，其他字符使用16x16绘制
    if (char_code >= FontConfig::ASCII_START && char_code <= FontConfig::ASCII_END) {
        draw_ascii_char(display, x, y, bitmap, color);
    } else {
        draw_flash_char(display, x, y, bitmap, color);
    }
}

template<typename DisplayDriver>
void FontRenderer<DisplayDriver>::draw_string(DisplayDriver& display, int x, int y, 
                                             const std::string& text, bool color) {
    draw_string(display, x, y, text.c_str(), color);
}

template<typename DisplayDriver>
void FontRenderer<DisplayDriver>::draw_string(DisplayDriver& display, int x, int y, 
                                             const char* text, bool color) {
    if (!font_source_ || !font_source_->is_valid() || !text) {
        return;
    }
    
    int current_x = x;
    const char* str = text;
    
    while (*str) {
        uint32_t char_code = decode_utf8_char(str);
        if (char_code == 0) {
            break;
        }
        
        draw_char(display, current_x, y, char_code, color);
        
        // 计算字符宽度
        if (char_code >= FontConfig::ASCII_START && char_code <= FontConfig::ASCII_END) {
            current_x += FontConfig::ASCII_FONT_WIDTH;
        } else {
            current_x += FontConfig::FLASH_FONT_WIDTH;
        }
    }
}

template<typename DisplayDriver>
int FontRenderer<DisplayDriver>::calculate_string_width(const std::string& text) const {
    return calculate_string_width(text.c_str());
}

template<typename DisplayDriver>
int FontRenderer<DisplayDriver>::calculate_string_width(const char* text) const {
    if (!font_source_ || !font_source_->is_valid() || !text) {
        return 0;
    }
    
    int width = 0;
    const char* str = text;
    
    while (*str) {
        uint32_t char_code = decode_utf8_char(str);
        if (char_code == 0) {
            break;
        }
        
        if (char_code >= FontConfig::ASCII_START && char_code <= FontConfig::ASCII_END) {
            width += FontConfig::ASCII_FONT_WIDTH;
        } else {
            width += FontConfig::FLASH_FONT_WIDTH;
        }
    }
    
    return width;
}

template<typename DisplayDriver>
uint32_t FontRenderer<DisplayDriver>::decode_utf8_char(const char*& str) const {
    if (!*str) return 0;
    
    const uint8_t* s = reinterpret_cast<const uint8_t*>(str);
    uint32_t codepoint = 0;
    
    if (s[0] < 0x80) {
        // ASCII字符
        codepoint = s[0];
        str += 1;
    } else if ((s[0] & 0xE0) == 0xC0) {
        // 2字节UTF-8
        if (s[1] && (s[1] & 0xC0) == 0x80) {
            codepoint = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
            str += 2;
        } else {
            str += 1;
        }
    } else if ((s[0] & 0xF0) == 0xE0) {
        // 3字节UTF-8
        if (s[1] && s[2] && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
            codepoint = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
            str += 3;
        } else {
            str += 1;
        }
    } else if ((s[0] & 0xF8) == 0xF0) {
        // 4字节UTF-8
        if (s[1] && s[2] && s[3] && 
            (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80) {
            codepoint = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | 
                       ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
            str += 4;
        } else {
            str += 1;
        }
    } else {
        // 无效UTF-8序列
        str += 1;
    }
    
    return codepoint;
}

template<typename DisplayDriver>
void FontRenderer<DisplayDriver>::draw_ascii_char(DisplayDriver& display, int x, int y, 
                                                 const std::vector<uint8_t>& bitmap, bool color) {
    if (bitmap.size() < FontConfig::ASCII_BYTES_PER_CHAR) {
        return;
    }
    
    // 定义颜色：true=白色，false=黑色
    uint16_t foreground_color = color ? 0xFFFF : 0x0000;  // 白色或黑色
    uint16_t background_color = color ? 0x0000 : 0xFFFF;  // 相反色
    
    for (int row = 0; row < FontConfig::ASCII_FONT_HEIGHT; row++) {
        uint8_t line_data = bitmap[row];
        for (int col = 0; col < FontConfig::ASCII_FONT_WIDTH; col++) {
            if (line_data & (0x80 >> col)) {
                display.drawPixel(x + col, y + row, foreground_color);
            } else {
                display.drawPixel(x + col, y + row, background_color);
            }
        }
    }
}

template<typename DisplayDriver>
void FontRenderer<DisplayDriver>::draw_flash_char(DisplayDriver& display, int x, int y, 
                                                 const std::vector<uint8_t>& bitmap, bool color) {
    if (bitmap.size() < FontConfig::FLASH_BYTES_PER_CHAR) {
        return;
    }
    
    // 定义颜色：true=白色，false=黑色
    uint16_t foreground_color = color ? 0xFFFF : 0x0000;  // 白色或黑色
    uint16_t background_color = color ? 0x0000 : 0xFFFF;  // 相反色
    
    for (int row = 0; row < FontConfig::FLASH_FONT_HEIGHT; row++) {
        uint16_t line_data = (bitmap[row * 2] << 8) | bitmap[row * 2 + 1];
        for (int col = 0; col < FontConfig::FLASH_FONT_WIDTH; col++) {
            if (line_data & (0x8000 >> col)) {
                display.drawPixel(x + col, y + row, foreground_color);
            } else {
                display.drawPixel(x + col, y + row, background_color);
            }
        }
    }
}

// ============================================================================
// FontManager 模板实现
// ============================================================================

template<typename DisplayDriver>
FontManager<DisplayDriver>::FontManager(uint32_t flash_address) : initialized_(false) {
    font_source_ = std::make_shared<HybridFontSource>(flash_address);
    renderer_ = std::make_unique<FontRenderer<DisplayDriver>>(font_source_);
    
    initialized_ = initialize(flash_address);
}

template<typename DisplayDriver>
bool FontManager<DisplayDriver>::initialize(uint32_t flash_address) {
    if (!font_source_) {
        font_source_ = std::make_shared<HybridFontSource>(flash_address);
    }
    
    if (!renderer_) {
        renderer_ = std::make_unique<FontRenderer<DisplayDriver>>(font_source_);
    }
    
    if (!font_source_->initialize(flash_address)) {
        printf("[FontManager] 字体源初始化失败\n");
        initialized_ = false;
        return false;
    }
    
    renderer_->set_font_source(font_source_);
    
    printf("[FontManager] 字体管理器初始化完成\n");
    initialized_ = true;
    return true;
}

template<typename DisplayDriver>
bool FontManager<DisplayDriver>::is_valid() const {
    return initialized_ && font_source_ && font_source_->is_valid() && renderer_;
}

template<typename DisplayDriver>
void FontManager<DisplayDriver>::draw_char(DisplayDriver& display, int x, int y, 
                                          uint32_t char_code, bool color) {
    if (renderer_) {
        renderer_->draw_char(display, x, y, char_code, color);
    }
}

template<typename DisplayDriver>
void FontManager<DisplayDriver>::draw_string(DisplayDriver& display, int x, int y, 
                                            const std::string& text, bool color) {
    if (renderer_) {
        renderer_->draw_string(display, x, y, text, color);
    }
}

template<typename DisplayDriver>
void FontManager<DisplayDriver>::draw_string(DisplayDriver& display, int x, int y, 
                                            const char* text, bool color) {
    if (renderer_) {
        renderer_->draw_string(display, x, y, text, color);
    }
}

template<typename DisplayDriver>
int FontManager<DisplayDriver>::get_string_width(const std::string& text) const {
    if (renderer_) {
        return renderer_->calculate_string_width(text);
    }
    return 0;
}

template<typename DisplayDriver>
int FontManager<DisplayDriver>::get_string_width(const char* text) const {
    if (renderer_) {
        return renderer_->calculate_string_width(text);
    }
    return 0;
}

template<typename DisplayDriver>
FontRenderer<DisplayDriver>& FontManager<DisplayDriver>::get_renderer() {
    return *renderer_;
}

template<typename DisplayDriver>
const FontRenderer<DisplayDriver>& FontManager<DisplayDriver>::get_renderer() const {
    return *renderer_;
}

template<typename DisplayDriver>
HybridFontSource& FontManager<DisplayDriver>::get_font_source() {
    return *font_source_;
}

template<typename DisplayDriver>
const HybridFontSource& FontManager<DisplayDriver>::get_font_source() const {
    return *font_source_;
}

template<typename DisplayDriver>
void FontManager<DisplayDriver>::print_status() const {
    printf("\n=== 字体管理器状态 ===\n");
    printf("初始化状态: %s\n", initialized_ ? "已初始化" : "未初始化");
    
    if (font_source_) {
        printf("字体源: %s\n", font_source_->get_type_name());
        printf("字体源有效性: %s\n", font_source_->is_valid() ? "有效" : "无效");
        printf("字体尺寸: %dx%d\n", font_source_->get_font_width(), font_source_->get_font_height());
    } else {
        printf("字体源: 未创建\n");
    }
    
    printf("渲染器: %s\n", renderer_ ? "已创建" : "未创建");
    printf("=====================\n\n");
}

} // namespace hybrid_font 