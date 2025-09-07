#include "flash_font_cache.hpp"
#include "unicode_ranges.h"
#include <cstdio>
#include <cstring>

namespace ili9488_font {

// 私有构造函数
FlashFontCache::FlashFontCache() 
    : flash_data_(nullptr), font_size_(0), initialized_(false) {
}

// 获取单例实例
FlashFontCache& FlashFontCache::get_instance() {
    static FlashFontCache instance;
    return instance;
}

// 使用Unicode范围表查找字符偏移
uint32_t FlashFontCache::get_char_offset(uint32_t unicode_code) const {
    return find_unicode_offset(unicode_code);
}

// 初始化Flash字体缓存
bool FlashFontCache::initialize(const uint8_t* flash_addr, int font_size) {
    if (!flash_addr) {
        return false;
    }
    
    if (font_size != 16 && font_size != 24) {
        return false;
    }
    
    flash_data_ = flash_addr;
    font_size_ = font_size;
    initialized_ = true;
    
    return true;
}

// 检查是否已初始化
bool FlashFontCache::is_initialized() const {
    return initialized_;
}

// 获取字体大小
int FlashFontCache::get_font_size() const {
    return font_size_;
}

// 从Flash中读取字符位图数据
std::vector<uint8_t> FlashFontCache::get_char_bitmap(uint16_t char_code) const {
    if (!initialized_) {
        return std::vector<uint8_t>();
    }
    
    // 获取字符在字体文件中的偏移
    uint32_t char_offset = get_char_offset(static_cast<uint32_t>(char_code));
    if (char_offset == UINT32_MAX) {
        // 不支持的字符，返回空格字符（偏移0）
        char_offset = 0;
    }
    
    // 计算字节偏移
    size_t bytes_per_char = (font_size_ == 16) ? BYTES_PER_CHAR_16 : BYTES_PER_CHAR_24;
    uint32_t byte_offset = sizeof(FontHeader) + char_offset * bytes_per_char;
    
    // 从Flash中读取数据
    const uint8_t* bitmap_data = flash_data_ + byte_offset;
    std::vector<uint8_t> bitmap(bytes_per_char);
    
    // 复制数据
    for (size_t i = 0; i < bytes_per_char; i++) {
        bitmap[i] = bitmap_data[i];
    }
    
    return bitmap;
}

// 验证Flash中的字体文件头
bool FlashFontCache::verify_font_header() const {
    if (!initialized_) {
        return false;
    }
    
    const FontHeader* header = reinterpret_cast<const FontHeader*>(flash_data_);
    
    // 检查版本号
    if (header->version != 1) {
        return false;
    }
    
    // 检查字符数量是否合理
    if (header->char_count < 1000 || header->char_count > 30000) {
        return false;
    }
    
    return true;
}

// 获取字体文件头信息
FontHeader FlashFontCache::get_font_header() const {
    FontHeader header = {0, 0};
    if (initialized_) {
        const FontHeader* flash_header = reinterpret_cast<const FontHeader*>(flash_data_);
        header = *flash_header;
    }
    return header;
}

// 检查Unicode字符是否支持
bool FlashFontCache::is_char_supported(uint32_t unicode_code) const {
    return is_unicode_supported(unicode_code);
}

// 获取Flash数据指针（用于调试）
const uint8_t* FlashFontCache::get_flash_data() const {
    return flash_data_;
}

// 重置缓存（用于重新初始化）
void FlashFontCache::reset() {
    flash_data_ = nullptr;
    font_size_ = 0;
    initialized_ = false;
}

// 调试功能：打印字符位图
void FlashFontCache::print_char_bitmap(uint16_t char_code) const {
    if (!initialized_) {
        printf("[ERROR] FlashFontCache未初始化\n");
        return;
    }
    
    std::vector<uint8_t> bitmap = get_char_bitmap(char_code);
    
    if (bitmap.empty()) {
        printf("[ERROR] 无法获取字符 0x%04X 的位图数据\n", char_code);
        return;
    }
    
    printf("\n=== 字符 0x%04X 点阵数据 ===\n", char_code);
    
    if (font_size_ == 16) {
        // 16x16字体，每行2字节
        for (int row = 0; row < 16; row++) {
            uint16_t row_data = (bitmap[row * 2] << 8) | bitmap[row * 2 + 1];
            printf("%02d: ", row);
            
            for (int col = 0; col < 16; col++) {
                char pixel = (row_data & (0x8000 >> col)) ? '#' : '.';
                printf("%c", pixel);
            }
            printf(" (0x%04X)\n", row_data);
        }
    } else if (font_size_ == 24) {
        // 24x24字体，每行3字节
        for (int row = 0; row < 24; row++) {
            uint32_t row_data = (bitmap[row * 3] << 16) | (bitmap[row * 3 + 1] << 8) | bitmap[row * 3 + 2];
            printf("%02d: ", row);
            
            for (int col = 0; col < 24; col++) {
                char pixel = (row_data & (0x800000 >> col)) ? '#' : '.';
                printf("%c", pixel);
            }
            printf(" (0x%06lX)\n", row_data);
        }
    }
    
    printf("========================\n");
}

// 调试功能：打印字体信息
void FlashFontCache::print_font_info() const {
    printf("\n=== Flash字体信息 ===\n");
    printf("初始化状态: %s\n", initialized_ ? "已初始化" : "未初始化");
    
    if (!initialized_) {
        printf("Flash地址: (null)\n");
        printf("字体大小: 0\n");
        printf("===================\n");
        return;
    }
    
    printf("Flash地址: %p\n", flash_data_);
    printf("字体大小: %dx%d\n", font_size_, font_size_);
    
    // 显示字体文件头信息
    if (verify_font_header()) {
        FontHeader header = get_font_header();
        printf("文件头验证: 通过\n");
        printf("版本号: %d\n", header.version);
        printf("字符总数: %d\n", header.char_count);
        printf("每字符字节数: %zu\n", (font_size_ == 16) ? BYTES_PER_CHAR_16 : BYTES_PER_CHAR_24);
    } else {
        printf("文件头验证: 失败\n");
    }
    
    // 显示前16字节原始数据
    printf("\n前16字节原始数据:\n");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", flash_data_[i]);
        if ((i + 1) % 8 == 0) printf(" ");
        if ((i + 1) % 16 == 0) printf("\n");
    }
    
    printf("===================\n");
}

// 调试功能：打印Unicode范围信息
void FlashFontCache::print_unicode_ranges() const {
    printf("\n=== Unicode范围信息 ===\n");
    printf("总范围数: %d\n", unicode_ranges_count);
    printf("总字符数: %ld\n", total_unicode_chars);
    printf("\n主要范围:\n");
    
    for (int i = 0; i < unicode_ranges_count && i < 10; i++) {
        const UnicodeRangeEntry* range = get_unicode_range(i);
        if (range) {
                    printf("[%2d] %-30s: 0x%04lX-0x%04lX (%5ld字符, 偏移%5ld) %s\n", 
               i, range->name, range->start, range->end, 
               range->count, range->offset,
               range->enabled ? "✓" : "✗");
        }
    }
    
    if (unicode_ranges_count > 10) {
        printf("... (还有%d个范围)\n", unicode_ranges_count - 10);
    }
    
    printf("======================\n");
}

} // namespace ili9488_font 