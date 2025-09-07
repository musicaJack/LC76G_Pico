#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include "flash_font_cache.hpp"

namespace hybrid_font {

/**
 * @brief 字体配置常量
 */
struct FontConfig {
    static constexpr int ASCII_FONT_WIDTH = 8;
    static constexpr int ASCII_FONT_HEIGHT = 16;
    static constexpr int ASCII_BYTES_PER_CHAR = 16;
    
    static constexpr int FLASH_FONT_WIDTH = 16;
    static constexpr int FLASH_FONT_HEIGHT = 16;
    static constexpr int FLASH_BYTES_PER_CHAR = 32;
    
    static constexpr uint32_t ASCII_START = 0x20;
    static constexpr uint32_t ASCII_END = 0x7E;
    
    static constexpr uint32_t FLASH_FONT_ADDRESS = 0x10100000;
};

/**
 * @brief 字体数据源抽象基类
 * 定义了字体数据源的统一接口
 */
class IFontDataSource {
public:
    virtual ~IFontDataSource() = default;
    
    /**
     * @brief 获取字符的位图数据
     * @param char_code Unicode字符代码
     * @return 位图数据向量，如果字符不支持则返回空向量
     */
    virtual std::vector<uint8_t> get_char_bitmap(uint32_t char_code) const = 0;
    
    /**
     * @brief 检查是否支持指定字符
     * @param char_code Unicode字符代码
     * @return true如果支持，false如果不支持
     */
    virtual bool is_char_supported(uint32_t char_code) const = 0;
    
    /**
     * @brief 获取字体宽度
     * @return 字体宽度（像素）
     */
    virtual int get_font_width() const = 0;
    
    /**
     * @brief 获取字体高度
     * @return 字体高度（像素）
     */
    virtual int get_font_height() const = 0;
    
    /**
     * @brief 获取每个字符的字节数
     * @return 每字符字节数
     */
    virtual int get_bytes_per_char() const = 0;
    
    /**
     * @brief 获取字体数据源类型名称
     * @return 类型名称字符串
     */
    virtual const char* get_type_name() const = 0;
    
    /**
     * @brief 验证字体数据源的有效性
     * @return true如果有效，false如果无效
     */
    virtual bool is_valid() const = 0;
};

/**
 * @brief ASCII字体数据源
 * 使用内置的8x16 ASCII字体
 */
class ASCIIFontSource : public IFontDataSource {
public:
    ASCIIFontSource();
    virtual ~ASCIIFontSource() = default;
    
    // IFontDataSource接口实现
    std::vector<uint8_t> get_char_bitmap(uint32_t char_code) const override;
    bool is_char_supported(uint32_t char_code) const override;
    int get_font_width() const override;
    int get_font_height() const override;
    int get_bytes_per_char() const override;
    const char* get_type_name() const override;
    bool is_valid() const override;
    
private:
    /**
     * @brief 获取ASCII字符的内置字体数据
     * @param ascii_code ASCII字符代码 (0x20-0x7E)
     * @return 指向字体数据的指针，如果不支持则返回nullptr
     */
    const uint8_t* get_ascii_font_data(uint8_t ascii_code) const;
    
    bool initialized_;
};

/**
 * @brief Flash字体数据源
 * 使用Flash中存储的16x16字体数据
 */
class FlashFontSource : public IFontDataSource {
public:
    /**
     * @brief 构造函数
     * @param flash_address Flash字体数据地址
     * @param font_size 字体大小（16或24）
     */
    explicit FlashFontSource(uint32_t flash_address = FontConfig::FLASH_FONT_ADDRESS, 
                            int font_size = 16);
    virtual ~FlashFontSource() = default;
    
    // IFontDataSource接口实现
    std::vector<uint8_t> get_char_bitmap(uint32_t char_code) const override;
    bool is_char_supported(uint32_t char_code) const override;
    int get_font_width() const override;
    int get_font_height() const override;
    int get_bytes_per_char() const override;
    const char* get_type_name() const override;
    bool is_valid() const override;
    
    /**
     * @brief 初始化Flash字体
     * @param flash_address Flash字体数据地址
     * @param font_size 字体大小
     * @return true如果初始化成功
     */
    bool initialize(uint32_t flash_address, int font_size);
    
    /**
     * @brief 获取Flash字体缓存实例
     * @return FlashFontCache引用
     */
    ili9488_font::FlashFontCache& get_cache();
    
    /**
     * @brief 获取Flash字体缓存实例（常量版本）
     * @return FlashFontCache常量引用
     */
    const ili9488_font::FlashFontCache& get_cache() const;
    
private:
    ili9488_font::FlashFontCache& cache_;
    uint32_t flash_address_;
    int font_size_;
    bool initialized_;
};

/**
 * @brief 混合字体数据源
 * 组合ASCII字体和Flash字体，提供统一的字体接口
 * ASCII字符使用内置8x16字体，非ASCII字符使用Flash 16x16字体
 */
class HybridFontSource : public IFontDataSource {
public:
    /**
     * @brief 构造函数
     * @param flash_address Flash字体数据地址
     */
    explicit HybridFontSource(uint32_t flash_address = FontConfig::FLASH_FONT_ADDRESS);
    virtual ~HybridFontSource() = default;
    
    // IFontDataSource接口实现
    std::vector<uint8_t> get_char_bitmap(uint32_t char_code) const override;
    bool is_char_supported(uint32_t char_code) const override;
    int get_font_width() const override;
    int get_font_height() const override;
    int get_bytes_per_char() const override;
    const char* get_type_name() const override;
    bool is_valid() const override;
    
    /**
     * @brief 初始化混合字体系统
     * @param flash_address Flash字体数据地址
     * @return true如果初始化成功
     */
    bool initialize(uint32_t flash_address = FontConfig::FLASH_FONT_ADDRESS);
    
    /**
     * @brief 获取ASCII字体数据源
     * @return ASCII字体数据源引用
     */
    const ASCIIFontSource& get_ascii_source() const;
    
    /**
     * @brief 获取Flash字体数据源
     * @return Flash字体数据源引用
     */
    const FlashFontSource& get_flash_source() const;
    
    /**
     * @brief 判断字符应该使用哪个字体源
     * @param char_code Unicode字符代码
     * @return true如果使用ASCII字体，false如果使用Flash字体
     */
    bool should_use_ascii_font(uint32_t char_code) const;
    
private:
    std::unique_ptr<ASCIIFontSource> ascii_source_;
    std::unique_ptr<FlashFontSource> flash_source_;
    bool initialized_;
};

} // namespace hybrid_font 