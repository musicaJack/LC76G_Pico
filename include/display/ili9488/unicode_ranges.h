// 自动生成的Unicode范围查找表
// 生成时间: 2025-06-22 15:00:54
// 源文件: unicode_export_tbl.json
// 
// 警告: 此文件由脚本自动生成，请勿手动修改！

#pragma once

#include <stdint.h>
#include <cstdio>

// Unicode范围结构体
struct UnicodeRangeEntry {
    const char* name;           // 范围名称
    bool enabled;              // 是否启用
    uint32_t start;            // 起始码点
    uint32_t end;              // 结束码点
    uint32_t count;            // 字符数量
    uint32_t offset;           // 在字体文件中的偏移位置
};

// Unicode范围查找表
static const UnicodeRangeEntry unicode_ranges[] = {
    {
        "Basic_Latin_(ASCII)",
        true,
        0x0020,
        0x007E,
        95,
        0
    },
    {
        "Latin-1_Supplement",
        true,
        0x00A0,
        0x00FF,
        96,
        95
    },
    {
        "General_Punctuation",
        true,
        0x2000,
        0x206F,
        112,
        191
    },
    {
        "Currency_Symbols",
        true,
        0x20A0,
        0x20CF,
        48,
        303
    },
    {
        "Letterlike_Symbols",
        true,
        0x2100,
        0x214F,
        80,
        351
    },
    {
        "Number_Forms_Roman_Numerals_Upper",
        true,
        0x2160,
        0x216B,
        12,
        431
    },
    {
        "Number_Forms_Roman_Numerals_Lower",
        true,
        0x2170,
        0x2179,
        10,
        443
    },
    {
        "Arrows",
        true,
        0x2190,
        0x21FF,
        112,
        453
    },
    {
        "Mathematical_Operators",
        true,
        0x2200,
        0x22FF,
        256,
        565
    },
    {
        "Box_Drawing",
        true,
        0x2500,
        0x257F,
        128,
        821
    },
    {
        "Block_Elements",
        true,
        0x2580,
        0x259F,
        32,
        949
    },
    {
        "Geometric_Shapes",
        true,
        0x25A0,
        0x25FF,
        96,
        981
    },
    {
        "Miscellaneous_Symbols",
        true,
        0x2600,
        0x26FF,
        256,
        1077
    },
    {
        "CJK_Squared_Symbols",
        true,
        0x338E,
        0x33D5,
        72,
        1333
    },
    {
        "CJK_Compatibility",
        true,
        0x3380,
        0x33DF,
        96,
        1405
    },
    {
        "CJK_Symbols_and_Punctuation",
        true,
        0x3000,
        0x303F,
        64,
        1501
    },
    {
        "Hiragana",
        true,
        0x3040,
        0x309F,
        96,
        1565
    },
    {
        "Katakana",
        true,
        0x30A0,
        0x30FF,
        96,
        1661
    },
    {
        "Bopomofo",
        true,
        0x3100,
        0x312F,
        48,
        1757
    },
    {
        "Bopomofo_Extended",
        true,
        0x31A0,
        0x31BF,
        32,
        1805
    },
    {
        "CJK_Unified_Ideographs",
        true,
        0x4E00,
        0x9FA5,
        20902,
        1837
    },
    {
        "Halfwidth_and_Fullwidth_Forms",
        true,
        0xFF00,
        0xFFEF,
        240,
        22739
    }
};

// 范围总数
static const int unicode_ranges_count = sizeof(unicode_ranges) / sizeof(unicode_ranges[0]);

// 总字符数
static const uint32_t total_unicode_chars = 22979;

// 查找Unicode字符在字体文件中的偏移位置
inline uint32_t find_unicode_offset(uint32_t unicode_code) {
    for (int i = 0; i < unicode_ranges_count; i++) {
        const UnicodeRangeEntry& range = unicode_ranges[i];
        if (range.enabled && unicode_code >= range.start && unicode_code <= range.end) {
            // 在范围内，计算偏移
            uint32_t relative_offset = unicode_code - range.start;
            return range.offset + relative_offset;
        }
    }
    return UINT32_MAX; // 未找到
}

// 检查Unicode字符是否受支持
inline bool is_unicode_supported(uint32_t unicode_code) {
    return find_unicode_offset(unicode_code) != UINT32_MAX;
}

// 获取指定索引的Unicode范围信息（用于调试）
inline const UnicodeRangeEntry* get_unicode_range(int index) {
    if (index >= 0 && index < unicode_ranges_count) {
        return &unicode_ranges[index];
    }
    return nullptr;
}

// 打印所有Unicode范围信息（用于调试）
inline void print_unicode_ranges() {
    for (int i = 0; i < unicode_ranges_count; i++) {
        const UnicodeRangeEntry& range = unicode_ranges[i];
        printf("[%d] %s: 0x%04lX-0x%04lX (%ld chars, offset %ld) %s\n", 
               i, range.name, range.start, range.end, range.count, range.offset,
               range.enabled ? "✓" : "✗");
    }
}
