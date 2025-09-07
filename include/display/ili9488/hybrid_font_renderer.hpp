#pragma once

#include "hybrid_font_system.hpp"
#include <string>
#include <memory>

namespace hybrid_font {

/**
 * @brief 字体渲染器模板类
 * 支持任意显示驱动类型，使用模板实现类型安全
 */
template<typename DisplayDriver>
class FontRenderer {
public:
    /**
     * @brief 构造函数
     * @param font_source 字体数据源
     */
    explicit FontRenderer(std::shared_ptr<IFontDataSource> font_source = nullptr);
    
    /**
     * @brief 设置字体数据源
     * @param font_source 字体数据源
     */
    void set_font_source(std::shared_ptr<IFontDataSource> font_source);
    
    /**
     * @brief 获取字体数据源
     * @return 字体数据源智能指针
     */
    std::shared_ptr<IFontDataSource> get_font_source() const;
    
    /**
     * @brief 绘制单个字符
     * @param display 显示驱动实例
     * @param x X坐标
     * @param y Y坐标
     * @param char_code Unicode字符代码
     * @param color 颜色（true=前景色，false=背景色）
     */
    void draw_char(DisplayDriver& display, int x, int y, uint32_t char_code, bool color);
    
    /**
     * @brief 绘制字符串
     * @param display 显示驱动实例
     * @param x X坐标
     * @param y Y坐标
     * @param text 字符串
     * @param color 颜色
     */
    void draw_string(DisplayDriver& display, int x, int y, const std::string& text, bool color);
    
    /**
     * @brief 绘制C风格字符串
     * @param display 显示驱动实例
     * @param x X坐标
     * @param y Y坐标
     * @param text C风格字符串
     * @param color 颜色
     */
    void draw_string(DisplayDriver& display, int x, int y, const char* text, bool color);
    
    /**
     * @brief 计算字符串显示宽度
     * @param text 字符串
     * @return 显示宽度（像素）
     */
    int calculate_string_width(const std::string& text) const;
    
    /**
     * @brief 计算C风格字符串显示宽度
     * @param text C风格字符串
     * @return 显示宽度（像素）
     */
    int calculate_string_width(const char* text) const;
    
private:
    /**
     * @brief 解码UTF-8字符
     * @param str 字符串指针（会被修改）
     * @return Unicode字符代码
     */
    uint32_t decode_utf8_char(const char*& str) const;
    
    /**
     * @brief 绘制ASCII字符（8x16）
     * @param display 显示驱动实例
     * @param x X坐标
     * @param y Y坐标
     * @param bitmap 位图数据
     * @param color 颜色
     */
    void draw_ascii_char(DisplayDriver& display, int x, int y, 
                        const std::vector<uint8_t>& bitmap, bool color);
    
    /**
     * @brief 绘制Flash字符（16x16）
     * @param display 显示驱动实例
     * @param x X坐标
     * @param y Y坐标
     * @param bitmap 位图数据
     * @param color 颜色
     */
    void draw_flash_char(DisplayDriver& display, int x, int y, 
                        const std::vector<uint8_t>& bitmap, bool color);
    
    std::shared_ptr<IFontDataSource> font_source_;
};

/**
 * @brief 字体管理器模板类
 * 提供高级字体管理功能，简化字体系统的使用
 */
template<typename DisplayDriver>
class FontManager {
public:
    /**
     * @brief 构造函数
     * @param flash_address Flash字体数据地址
     */
    explicit FontManager(uint32_t flash_address = FontConfig::FLASH_FONT_ADDRESS);
    
    /**
     * @brief 析构函数
     */
    ~FontManager() = default;
    
    /**
     * @brief 初始化字体管理器
     * @param flash_address Flash字体数据地址
     * @return true如果初始化成功
     */
    bool initialize(uint32_t flash_address = FontConfig::FLASH_FONT_ADDRESS);
    
    /**
     * @brief 检查字体管理器是否有效
     * @return true如果有效
     */
    bool is_valid() const;
    
    /**
     * @brief 绘制字符
     * @param display 显示驱动实例
     * @param x X坐标
     * @param y Y坐标
     * @param char_code Unicode字符代码
     * @param color 颜色
     */
    void draw_char(DisplayDriver& display, int x, int y, uint32_t char_code, bool color);
    
    /**
     * @brief 绘制字符串
     * @param display 显示驱动实例
     * @param x X坐标
     * @param y Y坐标
     * @param text 字符串
     * @param color 颜色
     */
    void draw_string(DisplayDriver& display, int x, int y, const std::string& text, bool color);
    
    /**
     * @brief 绘制C风格字符串
     * @param display 显示驱动实例
     * @param x X坐标
     * @param y Y坐标
     * @param text C风格字符串
     * @param color 颜色
     */
    void draw_string(DisplayDriver& display, int x, int y, const char* text, bool color);
    
    /**
     * @brief 计算字符串宽度
     * @param text 字符串
     * @return 宽度（像素）
     */
    int get_string_width(const std::string& text) const;
    
    /**
     * @brief 计算C风格字符串宽度
     * @param text C风格字符串
     * @return 宽度（像素）
     */
    int get_string_width(const char* text) const;
    
    /**
     * @brief 获取字体渲染器
     * @return 字体渲染器引用
     */
    FontRenderer<DisplayDriver>& get_renderer();
    
    /**
     * @brief 获取字体渲染器（常量版本）
     * @return 字体渲染器常量引用
     */
    const FontRenderer<DisplayDriver>& get_renderer() const;
    
    /**
     * @brief 获取混合字体数据源
     * @return 混合字体数据源引用
     */
    HybridFontSource& get_font_source();
    
    /**
     * @brief 获取混合字体数据源（常量版本）
     * @return 混合字体数据源常量引用
     */
    const HybridFontSource& get_font_source() const;
    
    /**
     * @brief 打印字体系统状态信息
     */
    void print_status() const;
    
private:
    std::shared_ptr<HybridFontSource> font_source_;
    std::unique_ptr<FontRenderer<DisplayDriver>> renderer_;
    bool initialized_;
};

} // namespace hybrid_font

// 包含模板实现
#include "hybrid_font_renderer.inl" 