#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

namespace ili9488_font {

// 字体文件头结构
struct FontHeader {
    uint16_t version;     // 版本号
    uint16_t char_count;  // 字符数量
};

// Flash字体缓存类
class FlashFontCache {
private:
    static constexpr size_t BYTES_PER_CHAR_16 = 32;  // 16x16字体每字符字节数
    static constexpr size_t BYTES_PER_CHAR_24 = 72;  // 24x24字体每字符字节数
    
    const uint8_t* flash_data_;     // Flash数据指针
    int font_size_;                 // 字体大小 (16 或 24)
    bool initialized_;              // 初始化状态
    
    // 私有构造函数（单例模式）
    FlashFontCache();
    
    // 禁用拷贝构造和赋值
    FlashFontCache(const FlashFontCache&) = delete;
    FlashFontCache& operator=(const FlashFontCache&) = delete;
    
    // 使用Unicode范围表查找字符偏移
    uint32_t get_char_offset(uint32_t unicode_code) const;
    
public:
    // 获取单例实例
    static FlashFontCache& get_instance();
    
    // 初始化Flash字体缓存
    bool initialize(const uint8_t* flash_addr, int font_size = 16);
    
    // 检查是否已初始化
    bool is_initialized() const;
    
    // 获取字体大小
    int get_font_size() const;
    
    // 从Flash中读取字符位图数据
    std::vector<uint8_t> get_char_bitmap(uint16_t char_code) const;
    
    // 验证Flash中的字体文件头
    bool verify_font_header() const;
    
    // 获取字体文件头信息
    FontHeader get_font_header() const;
    
    // 检查Unicode字符是否支持
    bool is_char_supported(uint32_t unicode_code) const;
    
    // 获取Flash数据指针（用于调试）
    const uint8_t* get_flash_data() const;
    
    // 重置缓存（用于重新初始化）
    void reset();
    
    // 调试功能：打印字符位图
    void print_char_bitmap(uint16_t char_code) const;
    
    // 调试功能：打印字体信息
    void print_font_info() const;
    
    // 调试功能：打印Unicode范围信息
    void print_unicode_ranges() const;
};

} // namespace ili9488_font 