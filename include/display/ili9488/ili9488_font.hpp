#pragma once

#include <cstdint>

namespace font {

/*
 * ILI9488 8x16 Font Library Header
 *
 * This file declares the API for accessing the 8x16 ASCII font library.
 * Based on ST73xx font architecture.
 *
 * Font data is generated from a TTF font or IBM_VGA_8x16.h, and each character occupies 16 bytes (8x16 pixels, 1 byte per row).
 * The font array contains 256 characters (ASCII 0~255), total 4096 bytes.
 *
 * Usage:
 *   - Use get_char_data(char c) to get a pointer to the 16-byte font data for character c.
 *   - Each byte represents one row, each bit is a pixel (1: on, 0: off).
 *
 * Example:
 *   const uint8_t* data = font::get_char_data('A');
 *   // data[0] ~ data[15] is the bitmap for 'A'
 *
 * Generation:
 *   - This file and the corresponding .cpp are adapted from ST73xx project.
 *   - The font data is extracted from IBM VGA font, and written in C++ array format.
 */

// Font size constants
constexpr int FONT_WIDTH = 8;
constexpr int FONT_HEIGHT = 16;
constexpr int FONT_SIZE = 4096;  // 16 * 256 characters

// Font data declaration
extern const uint8_t ILI9488_FONT[FONT_SIZE];

// Helper function to get character data
/**
 * Get pointer to 16-byte font data for character c.
 * @param c ASCII character
 * @return const uint8_t* pointer to 16 bytes (each row is 1 byte)
 */
inline const uint8_t* get_char_data(char c) {
    return &ILI9488_FONT[static_cast<unsigned char>(c) * FONT_HEIGHT];
}

} // namespace font

/*
// DEMO: Print the bitmap of a character
#include <cstdio>
int main() {
    const uint8_t* data = font::get_char_data('A');
    for (int i = 0; i < font::FONT_HEIGHT; ++i) {
        for (int b = 7; b >= 0; --b)
            putchar((data[i] & (1 << b)) ? '#' : '.');
        putchar('\n');
    }
    return 0;
}
*/ 