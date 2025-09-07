#include "hybrid_font_system.hpp"
#include "ili9488_font.hpp"
#include <cstdio>

namespace hybrid_font {

// ============================================================================
// ASCIIFontSource 实现
// ============================================================================

ASCIIFontSource::ASCIIFontSource() : initialized_(true) {
    // ASCII字体数据源总是可用的，使用内置字体
}

std::vector<uint8_t> ASCIIFontSource::get_char_bitmap(uint32_t char_code) const {
    if (!is_char_supported(char_code)) {
        return std::vector<uint8_t>();
    }
    
    const uint8_t* font_data = get_ascii_font_data(static_cast<uint8_t>(char_code));
    if (!font_data) {
        return std::vector<uint8_t>();
    }
    
    // 将字体数据复制到vector中
    std::vector<uint8_t> bitmap(font_data, font_data + FontConfig::ASCII_BYTES_PER_CHAR);
    return bitmap;
}

bool ASCIIFontSource::is_char_supported(uint32_t char_code) const {
    return char_code >= FontConfig::ASCII_START && char_code <= FontConfig::ASCII_END;
}

int ASCIIFontSource::get_font_width() const {
    return FontConfig::ASCII_FONT_WIDTH;
}

int ASCIIFontSource::get_font_height() const {
    return FontConfig::ASCII_FONT_HEIGHT;
}

int ASCIIFontSource::get_bytes_per_char() const {
    return FontConfig::ASCII_BYTES_PER_CHAR;
}

const char* ASCIIFontSource::get_type_name() const {
    return "ASCII Font Source";
}

bool ASCIIFontSource::is_valid() const {
    return initialized_;
}

const uint8_t* ASCIIFontSource::get_ascii_font_data(uint8_t ascii_code) const {
    if (!is_char_supported(ascii_code)) {
        return nullptr;
    }
    
    // 使用现有的font系统获取ASCII字体数据
    return font::get_char_data(ascii_code);
}

// ============================================================================
// FlashFontSource 实现
// ============================================================================

FlashFontSource::FlashFontSource(uint32_t flash_address, int font_size)
    : cache_(ili9488_font::FlashFontCache::get_instance()), 
      flash_address_(flash_address), 
      font_size_(font_size), 
      initialized_(false) {
    
    initialize(flash_address, font_size);
}

std::vector<uint8_t> FlashFontSource::get_char_bitmap(uint32_t char_code) const {
    if (!initialized_) {
        return std::vector<uint8_t>();
    }
    
    return cache_.get_char_bitmap(static_cast<uint16_t>(char_code));
}

bool FlashFontSource::is_char_supported(uint32_t char_code) const {
    if (!initialized_) {
        return false;
    }
    
    return cache_.is_char_supported(char_code);
}

int FlashFontSource::get_font_width() const {
    return FontConfig::FLASH_FONT_WIDTH;
}

int FlashFontSource::get_font_height() const {
    return FontConfig::FLASH_FONT_HEIGHT;
}

int FlashFontSource::get_bytes_per_char() const {
    return FontConfig::FLASH_BYTES_PER_CHAR;
}

const char* FlashFontSource::get_type_name() const {
    return "Flash Font Source";
}

bool FlashFontSource::is_valid() const {
    return initialized_ && cache_.is_initialized() && cache_.verify_font_header();
}

bool FlashFontSource::initialize(uint32_t flash_address, int font_size) {
    flash_address_ = flash_address;
    font_size_ = font_size;
    
    const uint8_t* font_data = reinterpret_cast<const uint8_t*>(flash_address);
    
    if (!cache_.initialize(font_data, font_size)) {
        printf("[FlashFontSource] 初始化失败: Flash地址 0x%08lX\n", flash_address);
        initialized_ = false;
        return false;
    }
    
    if (!cache_.verify_font_header()) {
        printf("[FlashFontSource] 字体文件头验证失败\n");
        initialized_ = false;
        return false;
    }
    
    printf("[FlashFontSource] 初始化成功: Flash地址 0x%08lX, 字体大小 %dx%d\n", 
           flash_address, font_size, font_size);
    
    initialized_ = true;
    return true;
}

ili9488_font::FlashFontCache& FlashFontSource::get_cache() {
    return cache_;
}

const ili9488_font::FlashFontCache& FlashFontSource::get_cache() const {
    return cache_;
}

// ============================================================================
// HybridFontSource 实现
// ============================================================================

HybridFontSource::HybridFontSource(uint32_t flash_address) : initialized_(false) {
    ascii_source_ = std::make_unique<ASCIIFontSource>();
    flash_source_ = std::make_unique<FlashFontSource>(flash_address);
    
    initialize(flash_address);
}

std::vector<uint8_t> HybridFontSource::get_char_bitmap(uint32_t char_code) const {
    if (!initialized_) {
        return std::vector<uint8_t>();
    }
    
    if (should_use_ascii_font(char_code)) {
        return ascii_source_->get_char_bitmap(char_code);
    } else {
        return flash_source_->get_char_bitmap(char_code);
    }
}

bool HybridFontSource::is_char_supported(uint32_t char_code) const {
    if (!initialized_) {
        return false;
    }
    
    if (should_use_ascii_font(char_code)) {
        return ascii_source_->is_char_supported(char_code);
    } else {
        return flash_source_->is_char_supported(char_code);
    }
}

int HybridFontSource::get_font_width() const {
    // 混合字体系统返回最大宽度
    return FontConfig::FLASH_FONT_WIDTH;
}

int HybridFontSource::get_font_height() const {
    // 混合字体系统返回最大高度
    return FontConfig::FLASH_FONT_HEIGHT;
}

int HybridFontSource::get_bytes_per_char() const {
    // 返回最大字节数
    return FontConfig::FLASH_BYTES_PER_CHAR;
}

const char* HybridFontSource::get_type_name() const {
    return "Hybrid Font Source (ASCII + Flash)";
}

bool HybridFontSource::is_valid() const {
    return initialized_ && 
           ascii_source_ && ascii_source_->is_valid() &&
           flash_source_ && flash_source_->is_valid();
}

bool HybridFontSource::initialize(uint32_t flash_address) {
    if (!ascii_source_ || !flash_source_) {
        printf("[HybridFontSource] 字体源创建失败\n");
        initialized_ = false;
        return false;
    }
    
    // ASCII字体源总是可用的
    if (!ascii_source_->is_valid()) {
        printf("[HybridFontSource] ASCII字体源无效\n");
        initialized_ = false;
        return false;
    }
    
    // 初始化Flash字体源
    if (!flash_source_->initialize(flash_address, 16)) {
        printf("[HybridFontSource] Flash字体源初始化失败\n");
        initialized_ = false;
        return false;
    }
    
    printf("[HybridFontSource] 混合字体系统初始化成功\n");
    printf("  - ASCII字体: %s (%dx%d)\n", 
           ascii_source_->get_type_name(),
           ascii_source_->get_font_width(), 
           ascii_source_->get_font_height());
    printf("  - Flash字体: %s (%dx%d)\n", 
           flash_source_->get_type_name(),
           flash_source_->get_font_width(), 
           flash_source_->get_font_height());
    
    initialized_ = true;
    return true;
}

const ASCIIFontSource& HybridFontSource::get_ascii_source() const {
    return *ascii_source_;
}

const FlashFontSource& HybridFontSource::get_flash_source() const {
    return *flash_source_;
}

bool HybridFontSource::should_use_ascii_font(uint32_t char_code) const {
    // ASCII字符范围使用ASCII字体
    return char_code >= FontConfig::ASCII_START && char_code <= FontConfig::ASCII_END;
}

} // namespace hybrid_font 