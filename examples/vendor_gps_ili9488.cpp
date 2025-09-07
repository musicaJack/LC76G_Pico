/**
 * @file vendor_gps_ili9488.cpp
 * @brief GPS数据显示示例 - 使用ILI9488显示屏
 * 
 * 功能说明：
 * - 接收L76X GPS模块数据
 * - 在ILI9488显示屏上显示GPS信息
 * - 支持坐标转换（WGS84 -> 百度/谷歌坐标）
 * - 显示卫星信号强度（模拟）
 * - 实时更新GPS状态
 * 
 * 硬件连接：
 * - GPS模块：UART连接
 * - ILI9488显示：SPI连接
 * - 引脚配置：参见pin_config.hpp
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

// C++头文件
#include "ili9488_driver.hpp"
#include "pico_ili9488_gfx.hpp"
#include "ili9488_colors.hpp"
#include "ili9488_font.hpp"
#include "pin_config.hpp"

extern "C" {
#include "gps/vendor_gps_parser.h"
}

// 使用C++命名空间
using namespace ili9488;
using namespace pico_ili9488_gfx;

// =============================================================================
// 显示配置常量
// =============================================================================

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   480

// 颜色定义 (RGB888格式)
#define COLOR_BLACK     0x000000
#define COLOR_WHITE     0xFFFFFF
#define COLOR_RED       0xFF0000
#define COLOR_GREEN     0x00FF00
#define COLOR_BLUE      0x0000FF
#define COLOR_YELLOW    0xFFFF00
#define COLOR_CYAN      0x00FFFF
#define COLOR_MAGENTA   0xFF00FF
#define COLOR_GRAY      0x808080
#define COLOR_DARK_GRAY 0x404040
#define COLOR_LIGHT_GRAY 0xC0C0C0

// 界面布局参数
#define HEADER_HEIGHT   40
#define LINE_HEIGHT     24
#define MARGIN_X        10
#define MARGIN_Y        50
#define DATA_START_Y    (HEADER_HEIGHT + MARGIN_Y)
#define STATUS_Y        (SCREEN_HEIGHT - 80)

// 区域定义
#define COORDINATES_Y       DATA_START_Y
#define COORDINATES_HEIGHT  (8 * LINE_HEIGHT)  // 8行GPS信息
#define SIGNAL_BARS_Y       (DATA_START_Y + 8 * LINE_HEIGHT + 30)
#define SIGNAL_BARS_HEIGHT  100
#define STATUS_BAR_HEIGHT   80

// GPS数据更新间隔（毫秒）
#define GPS_UPDATE_INTERVAL 1000
#define DISPLAY_REFRESH_INTERVAL 500
#define STATUS_UPDATE_INTERVAL 2000  // 状态栏更新间隔：2秒

// =============================================================================
// GPS配置常量（使用pin_config.hpp中的定义）
// =============================================================================
// 注意：GPS配置常量在pin_config.hpp中定义，此处不再重复定义

// =============================================================================
// 全局变量
// =============================================================================

// 显示驱动实例
static ILI9488Driver* driver = nullptr;
static PicoILI9488GFX<ILI9488Driver>* gfx = nullptr;

// GPS数据缓存
static GNRMC current_gps_data;
static GNRMC previous_gps_data;  // 用于比较变化
static bool gps_data_updated = false;
static uint32_t last_gps_update = 0;
static uint32_t last_display_refresh = 0;
static uint32_t last_status_update = 0;  // 状态栏上次更新时间

// GPS调试和统计信息
static uint32_t packet_count = 0;
static uint32_t valid_fix_count = 0;
static bool enable_debug = true;  // 启用调试输出

// 系统状态
static bool display_initialized = false;
static uint32_t system_start_time = 0;

// =============================================================================
// 显示驱动初始化
// =============================================================================

bool init_ili9488_display() {
    printf("正在初始化ILI9488显示驱动...\n");
    
    // 创建驱动实例
    driver = new ILI9488Driver(ILI9488_GET_SPI_CONFIG());
    gfx = new PicoILI9488GFX<ILI9488Driver>(*driver, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // 初始化驱动
    if (!driver->initialize()) {
        printf("错误：ILI9488驱动初始化失败\n");
        return false;
    }
    
    // 设置屏幕方向和背光
    driver->setRotation(Rotation::Portrait_180);  // 旋转180度
    driver->setBacklight(true);
    
    printf("ILI9488显示驱动初始化成功\n");
    return true;
}

// =============================================================================
// 图形绘制函数封装
// =============================================================================

void draw_filled_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
    if (gfx) {
        gfx->fillRect(x, y, w, h, color);
    }
}

void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
    if (gfx) {
        gfx->drawRect(x, y, w, h, color);
    }
}

void draw_hline(uint16_t x, uint16_t y, uint16_t w, uint32_t color) {
    if (gfx) {
        gfx->drawFastHLine(x, y, w, color);
    }
}

void draw_filled_circle(uint16_t x, uint16_t y, uint16_t r, uint32_t color) {
    if (gfx) {
        gfx->fillCircle(x, y, r, color);
    }
}

void draw_string(uint16_t x, uint16_t y, const char* str, uint32_t color, uint32_t bg_color) {
    if (driver && str) {
        driver->drawString(x, y, str, color, bg_color);
    }
}

void fill_screen(uint32_t color) {
    if (driver) {
        // 转换RGB888到RGB565
        uint16_t color565 = ((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) | ((color >> 3) & 0x001F);
        driver->fillScreen(color565);
    }
}

// =============================================================================
// GPS数据处理函数
// =============================================================================

/**
 * @brief 坐标转换：WGS84 -> 百度坐标系
 */
void wgs84_to_bd09(double wgs_lat, double wgs_lon, double* bd_lat, double* bd_lon) {
    // 简化的坐标转换算法
    const double x_offset = 0.0065;
    const double y_offset = 0.006;
    const double z_offset = 0.00002 * sin(wgs_lat * M_PI / 180.0 * 3000.0);
    
    *bd_lat = wgs_lat + y_offset + z_offset;
    *bd_lon = wgs_lon + x_offset + z_offset;
}

/**
 * @brief 坐标转换：WGS84 -> 谷歌坐标系
 */
void wgs84_to_gcj02(double wgs_lat, double wgs_lon, double* gcj_lat, double* gcj_lon) {
    // 简化的坐标转换算法
    const double offset_lat = 0.00669342162296594323;
    const double offset_lon = 0.006693421622965943;
    
    *gcj_lat = wgs_lat + offset_lat;
    *gcj_lon = wgs_lon + offset_lon;
}

// 删除detect_data_changes函数，不再需要复杂的区域检测

/**
 * @brief 更新GPS数据（参考vendor_gps_display.c的调试机制）
 */
void update_gps_data() {
    GNRMC new_data = vendor_gps_get_gnrmc();
    
    // 保存之前的数据
    memcpy(&previous_gps_data, &current_gps_data, sizeof(GNRMC));
    
    // 增加数据包计数
    packet_count++;
    
    // 检查是否获得有效时间数据
    bool got_time_data = (new_data.Time_H > 0 || new_data.Time_M > 0 || new_data.Time_S > 0);
    bool got_valid_data = false;
    
    if (got_time_data) {
        if (enable_debug && packet_count % 10 == 0) {
            printf("GPS时间数据: %02d:%02d:%02d\n", 
                   new_data.Time_H, new_data.Time_M, new_data.Time_S);
        }
        
        // 检查数据是否有效（严格验证坐标）
        if (new_data.Status == 1) {
            if (fabs(new_data.Lat) > 0.0001 && fabs(new_data.Lon) > 0.0001) {
                if (enable_debug) {
                    printf("GPS有效数据: 状态=%d, 纬度=%.6f, 经度=%.6f\n", 
                           new_data.Status, new_data.Lat, new_data.Lon);
                }
                got_valid_data = true;
                valid_fix_count++;
            } else {
                if (enable_debug) {
                    printf("GPS状态有效但坐标接近零: 纬度=%.6f, 经度=%.6f\n", 
                           new_data.Lat, new_data.Lon);
                }
            }
        }
        
        if (got_valid_data) {
            // 输出GPS数据概要（每10个数据包输出一次）
            if (enable_debug && packet_count % 10 == 0) {
                printf("GPS: 坐标=%0.6f,%0.6f 速度=%.1fkm/h 航向=%.1f° 时间=%02d:%02d:%02d\n", 
                       new_data.Lat, new_data.Lon, 
                       new_data.Speed, new_data.Course,
                       new_data.Time_H, new_data.Time_M, new_data.Time_S);
            }
        } else {
            if (enable_debug && packet_count % 10 == 0) {
                printf("GPS: 等待定位中... 时间=%02d:%02d:%02d 日期=%s\n", 
                       new_data.Time_H, new_data.Time_M, new_data.Time_S,
                       new_data.Date);
            }
        }
    } else {
        if (enable_debug && packet_count % 20 == 0) {
            printf("GPS: 未获得有效时间数据，数据包计数=%lu\n", packet_count);
        }
    }
    
    // 检查是否有新的定位数据或时间数据
    if (got_time_data || memcmp(&new_data, &current_gps_data, sizeof(GNRMC)) != 0) {
        memcpy(&current_gps_data, &new_data, sizeof(GNRMC));
        gps_data_updated = true;
        last_gps_update = to_ms_since_boot(get_absolute_time());
    }
}

/**
 * @brief 清除指定区域
 */
void clear_area(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    draw_filled_rect(x, y, w, h, COLOR_BLACK);
}

// =============================================================================
// 界面绘制函数
// =============================================================================

/**
 * @brief 绘制标题栏
 */
void draw_header() {
    // 背景
    draw_filled_rect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BLUE);
    
    // 标题文字
    draw_string(SCREEN_WIDTH/2 - 80, 12, "GPS Position Monitor", COLOR_WHITE, COLOR_BLUE);
    
    // 分隔线
    draw_hline(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_WHITE);
}

/**
 * @brief 绘制GPS坐标信息（局部刷新版本）
 */
void draw_gps_coordinates(uint16_t start_y, bool clear_first = false) {
    static char prev_lat[32] = {0};
    static char prev_lon[32] = {0};
    static char prev_alt[32] = {0};
    static char prev_speed[32] = {0};
    static char prev_course[32] = {0};
    static char prev_satellites[16] = {0};
    static char prev_hdop[16] = {0};
    static char prev_status[64] = {0};
    static bool prev_fix_state = false;
    
    uint16_t y = start_y;
    
    // 如果需要，先清除区域（仅在第一次或强制清除时）
    if (clear_first) {
        clear_area(0, start_y, SCREEN_WIDTH, COORDINATES_HEIGHT);
        
        // 绘制固定标签
        draw_string(MARGIN_X, y, "Latitude : ", COLOR_WHITE, COLOR_BLACK);
        y += LINE_HEIGHT;
        draw_string(MARGIN_X, y, "Longitude: ", COLOR_WHITE, COLOR_BLACK);
        y += LINE_HEIGHT;
        draw_string(MARGIN_X, y, "Altitude : ", COLOR_WHITE, COLOR_BLACK);
        y += LINE_HEIGHT;
        draw_string(MARGIN_X, y, "Speed    : ", COLOR_WHITE, COLOR_BLACK);
        y += LINE_HEIGHT;
        draw_string(MARGIN_X, y, "Course   : ", COLOR_WHITE, COLOR_BLACK);
        y += LINE_HEIGHT;
        draw_string(MARGIN_X, y, "Satellites: ", COLOR_WHITE, COLOR_BLACK);
        y += LINE_HEIGHT;
        draw_string(MARGIN_X, y, "HDOP     : ", COLOR_WHITE, COLOR_BLACK);
        y += LINE_HEIGHT;
        draw_string(MARGIN_X, y, "Status   : ", COLOR_WHITE, COLOR_BLACK);
        return;
    }
    
    // 准备新数据字符串
    char new_lat[32], new_lon[32], new_alt[32], new_speed[32], new_course[32];
    char new_satellites[16], new_hdop[16], new_status[64];
    
    // 格式化新数据
    char lat_dir = (current_gps_data.Lat >= 0) ? 'N' : 'S';
    char lon_dir = (current_gps_data.Lon >= 0) ? 'E' : 'W';
    
    snprintf(new_lat, sizeof(new_lat), "%.6f %c", fabs(current_gps_data.Lat), lat_dir);
    snprintf(new_lon, sizeof(new_lon), "%.6f %c", fabs(current_gps_data.Lon), lon_dir);
    snprintf(new_alt, sizeof(new_alt), "%.1f m", current_gps_data.Altitude);
    snprintf(new_speed, sizeof(new_speed), "%.1f km/h", current_gps_data.Speed);
    snprintf(new_course, sizeof(new_course), "%.2f", current_gps_data.Course);
    
    int satellites = (current_gps_data.Status == 1) ? 10 : 0;
    float hdop = (current_gps_data.Status == 1) ? 1.4f : 0.0f;
    snprintf(new_satellites, sizeof(new_satellites), "%d", satellites);
    snprintf(new_hdop, sizeof(new_hdop), "%.1f", hdop);
    
    if (current_gps_data.Status == 1) {
        snprintf(new_status, sizeof(new_status), "Fix - %02d:%02d:%02d UTC",
                 current_gps_data.Time_H, current_gps_data.Time_M, current_gps_data.Time_S);
    } else {
        strcpy(new_status, "No Signal");
    }
    
    // 检查状态变化
    bool fix_state_changed = (prev_fix_state != (current_gps_data.Status == 1));
    
    // 更新纬度显示
    if (strcmp(new_lat, prev_lat) != 0 || fix_state_changed) {
        draw_filled_rect(MARGIN_X + 80, start_y, 200, 12, COLOR_BLACK);
        draw_string(MARGIN_X + 80, start_y, new_lat, COLOR_WHITE, COLOR_BLACK);
        strcpy(prev_lat, new_lat);
    }
    
    // 更新经度显示
    if (strcmp(new_lon, prev_lon) != 0 || fix_state_changed) {
        draw_filled_rect(MARGIN_X + 80, start_y + LINE_HEIGHT, 200, 12, COLOR_BLACK);
        draw_string(MARGIN_X + 80, start_y + LINE_HEIGHT, new_lon, COLOR_WHITE, COLOR_BLACK);
        strcpy(prev_lon, new_lon);
    }
    
    // 更新高度显示
    if (strcmp(new_alt, prev_alt) != 0 || fix_state_changed) {
        draw_filled_rect(MARGIN_X + 80, start_y + 2 * LINE_HEIGHT, 200, 12, COLOR_BLACK);
        draw_string(MARGIN_X + 80, start_y + 2 * LINE_HEIGHT, new_alt, COLOR_WHITE, COLOR_BLACK);
        strcpy(prev_alt, new_alt);
    }
    
    // 更新速度显示
    if (strcmp(new_speed, prev_speed) != 0 || fix_state_changed) {
        draw_filled_rect(MARGIN_X + 80, start_y + 3 * LINE_HEIGHT, 200, 12, COLOR_BLACK);
        draw_string(MARGIN_X + 80, start_y + 3 * LINE_HEIGHT, new_speed, COLOR_WHITE, COLOR_BLACK);
        strcpy(prev_speed, new_speed);
    }
    
    // 更新航向显示
    if (strcmp(new_course, prev_course) != 0 || fix_state_changed) {
        draw_filled_rect(MARGIN_X + 80, start_y + 4 * LINE_HEIGHT, 200, 12, COLOR_BLACK);
        draw_string(MARGIN_X + 80, start_y + 4 * LINE_HEIGHT, new_course, COLOR_WHITE, COLOR_BLACK);
        strcpy(prev_course, new_course);
    }
    
    // 更新卫星数量显示
    if (strcmp(new_satellites, prev_satellites) != 0 || fix_state_changed) {
        draw_filled_rect(MARGIN_X + 80, start_y + 5 * LINE_HEIGHT, 200, 12, COLOR_BLACK);
        draw_string(MARGIN_X + 80, start_y + 5 * LINE_HEIGHT, new_satellites, COLOR_GREEN, COLOR_BLACK);
        strcpy(prev_satellites, new_satellites);
    }
    
    // 更新HDOP显示
    if (strcmp(new_hdop, prev_hdop) != 0 || fix_state_changed) {
        draw_filled_rect(MARGIN_X + 80, start_y + 6 * LINE_HEIGHT, 200, 12, COLOR_BLACK);
        draw_string(MARGIN_X + 80, start_y + 6 * LINE_HEIGHT, new_hdop, COLOR_WHITE, COLOR_BLACK);
        strcpy(prev_hdop, new_hdop);
    }
    
    // 更新状态显示
    if (strcmp(new_status, prev_status) != 0 || fix_state_changed) {
        draw_filled_rect(MARGIN_X + 80, start_y + 7 * LINE_HEIGHT, 200, 12, COLOR_BLACK);
        uint32_t status_color = (current_gps_data.Status == 1) ? COLOR_GREEN : COLOR_RED;
        draw_string(MARGIN_X + 80, start_y + 7 * LINE_HEIGHT, new_status, status_color, COLOR_BLACK);
        strcpy(prev_status, new_status);
    }
    
    // 更新定位状态跟踪变量
    prev_fix_state = (current_gps_data.Status == 1);
}

/**
 * @brief 删除原来的详细信息函数，因为已经合并到坐标显示中
 */

/**
 * @brief 绘制卫星信号强度图
 */
void draw_satellite_signal_bars(uint16_t start_y, bool clear_first = false) {
    const uint16_t bar_width = 20;
    const uint16_t bar_spacing = 25;
    const uint16_t max_bar_height = 60;
    const uint16_t base_y = start_y + max_bar_height;
    
    // 如果需要，先清除区域
    if (clear_first) {
        clear_area(0, start_y - 25, SCREEN_WIDTH, SIGNAL_BARS_HEIGHT);
    }
    
    // 标题
    draw_string(MARGIN_X, start_y - 20, "Satellite Signal", COLOR_WHITE, COLOR_BLACK);
    
    // 绘制信号强度条（模拟数据）
    for (int i = 0; i < 8; i++) {
        uint16_t x = MARGIN_X + i * bar_spacing;
        
        // 模拟信号强度（基于GPS状态）
        uint8_t signal_strength = 0;
        if (current_gps_data.Status == 1 && i < 10) {
            signal_strength = 30 + (rand() % 50); // 30-80的信号强度
        }
        
        if (signal_strength > 0) {
            uint16_t bar_height = (signal_strength * max_bar_height) / 100;
            uint32_t bar_color = COLOR_GREEN;
            
            if (signal_strength < 30) bar_color = COLOR_RED;
            else if (signal_strength < 50) bar_color = COLOR_YELLOW;
            
            draw_filled_rect(x, base_y - bar_height, bar_width, bar_height, bar_color);
        }
        
        // 绘制边框
        draw_rect(x, base_y - max_bar_height, bar_width, max_bar_height, COLOR_GRAY);
        
        // 卫星编号
        char sat_num[4];
        snprintf(sat_num, sizeof(sat_num), "%d", i + 1);
        draw_string(x + 6, base_y + 5, sat_num, COLOR_WHITE, COLOR_BLACK);
    }
}

/**
 * @brief 绘制状态栏
 */
// 状态栏字符级更新的静态变量
static char prev_uptime_str[20] = {0};
static char prev_gps_update_str[20] = {0};
static char prev_utc_time_str[15] = {0};
static char prev_date_str[15] = {0};
static bool status_bar_initialized = false;

/**
 * @brief 智能更新时间字符串 - 保持hh:mm:ss格式
 * @param x 起始X坐标
 * @param y Y坐标 
 * @param new_str 新的时间字符串 (格式: hh:mm:ss)
 * @param prev_str 之前的时间字符串
 * @param color 文字颜色
 */
void update_time_string_precise(uint16_t x, uint16_t y, const char* new_str, char* prev_str, uint32_t color) {
    // 如果是第一次显示或长度不同，完整重绘
    if (strlen(prev_str) == 0 || strlen(new_str) != strlen(prev_str)) {
        draw_filled_rect(x, y, 72, 12, COLOR_BLACK);  // 足够宽度显示 hh:mm:ss
        draw_string(x, y, new_str, color, COLOR_BLACK);
        strcpy(prev_str, new_str);
        return;
    }
    
    // 对于标准时间格式 hh:mm:ss (8个字符)，按位置更新
    if (strlen(new_str) == 8 && new_str[2] == ':' && new_str[5] == ':') {
        // 小时位 (位置0,1)
        if (new_str[0] != prev_str[0] || new_str[1] != prev_str[1]) {
            draw_filled_rect(x, y, 16, 12, COLOR_BLACK);  // 清除小时区域
            char hour_str[3] = {new_str[0], new_str[1], '\0'};
            draw_string(x, y, hour_str, color, COLOR_BLACK);
        }
        
        // 分钟位 (位置3,4)
        if (new_str[3] != prev_str[3] || new_str[4] != prev_str[4]) {
            draw_filled_rect(x + 24, y, 16, 12, COLOR_BLACK);  // 清除分钟区域
            char min_str[3] = {new_str[3], new_str[4], '\0'};
            draw_string(x + 24, y, min_str, color, COLOR_BLACK);
        }
        
        // 秒位 (位置6,7) - 最常更新的部分
        if (new_str[6] != prev_str[6] || new_str[7] != prev_str[7]) {
            draw_filled_rect(x + 48, y, 16, 12, COLOR_BLACK);  // 清除秒区域
            char sec_str[3] = {new_str[6], new_str[7], '\0'};
            draw_string(x + 48, y, sec_str, color, COLOR_BLACK);
        }
        
        // 确保冒号显示正确 (位置2,5)
        if (new_str[2] != prev_str[2]) {
            draw_filled_rect(x + 16, y, 8, 12, COLOR_BLACK);
            draw_string(x + 16, y, ":", color, COLOR_BLACK);
        }
        if (new_str[5] != prev_str[5]) {
            draw_filled_rect(x + 40, y, 8, 12, COLOR_BLACK);
            draw_string(x + 40, y, ":", color, COLOR_BLACK);
        }
    } else {
        // 非标准格式，完整重绘
        draw_filled_rect(x, y, 72, 12, COLOR_BLACK);
        draw_string(x, y, new_str, color, COLOR_BLACK);
    }
    
    strcpy(prev_str, new_str);
}

void draw_status_bar(bool clear_first = false) {
    char buffer[64];
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    uint32_t uptime_seconds = (current_time - system_start_time) / 1000;
    
    // 如果需要，先清除区域并初始化背景
    if (clear_first || !status_bar_initialized) {
        clear_area(0, STATUS_Y, SCREEN_WIDTH, 50);  
        // 使用黑色背景，不再绘制灰色矩形
        status_bar_initialized = true;
        
        // 绘制静态标签（只在初始化时绘制一次）
        draw_string(MARGIN_X, STATUS_Y + 10, "Uptime:", COLOR_WHITE, COLOR_BLACK);
        if (current_gps_data.Status > 0) {
            draw_string(MARGIN_X, STATUS_Y + 25, "UTC:", COLOR_CYAN, COLOR_BLACK);
        }
    }
    
    // 更新运行时间（精确到字符级别）
    char uptime_str[20];
    snprintf(uptime_str, sizeof(uptime_str), "%02d:%02d:%02d", 
             uptime_seconds / 3600, (uptime_seconds / 60) % 60, uptime_seconds % 60);
    
    if (strcmp(uptime_str, prev_uptime_str) != 0) {
        update_time_string_precise(MARGIN_X + 56, STATUS_Y + 10, uptime_str, prev_uptime_str, COLOR_WHITE);
    }
    
    // 更新GPS数据状态（右侧）- 减少更新频率
    static uint32_t last_gps_status_update = 0;
    if (current_time - last_gps_status_update > 2000) {  // 每2秒才更新GPS状态
        char gps_update_str[20];
        if (gps_data_updated) {
            uint32_t seconds_since_update = (current_time - last_gps_update) / 1000;
            snprintf(gps_update_str, sizeof(gps_update_str), "%ds ago", seconds_since_update);
            
            if (strcmp(gps_update_str, prev_gps_update_str) != 0) {
                // 清除更新状态标签和数字区域
                draw_filled_rect(SCREEN_WIDTH - 130, STATUS_Y + 10, 130, 12, COLOR_BLACK);
                // 绘制完整的更新状态
                snprintf(buffer, sizeof(buffer), "Updated: %s", gps_update_str);
                draw_string(SCREEN_WIDTH - 130, STATUS_Y + 10, buffer, COLOR_GREEN, COLOR_BLACK);
                strcpy(prev_gps_update_str, gps_update_str);
            }
        } else {
            if (strcmp("Waiting", prev_gps_update_str) != 0) {
                draw_filled_rect(SCREEN_WIDTH - 100, STATUS_Y + 10, 100, 12, COLOR_BLACK);
                draw_string(SCREEN_WIDTH - 100, STATUS_Y + 10, "Waiting...", COLOR_YELLOW, COLOR_BLACK);
                strcpy(prev_gps_update_str, "Waiting");
            }
        }
        last_gps_status_update = current_time;
    }
    
    // 更新UTC时间（精确到字符级别）
    if (current_gps_data.Status > 0) {
        char utc_time_str[15];
        snprintf(utc_time_str, sizeof(utc_time_str), "%02d:%02d:%02d", 
                 current_gps_data.Time_H, current_gps_data.Time_M, current_gps_data.Time_S);
        
        if (strcmp(utc_time_str, prev_utc_time_str) != 0) {
            update_time_string_precise(MARGIN_X + 32, STATUS_Y + 25, utc_time_str, prev_utc_time_str, COLOR_CYAN);
        }
    }
    
    // 更新日期（通常不经常变化）
    if (current_gps_data.Status > 0 && current_gps_data.Date[0] != '\0') {
        if (strcmp(current_gps_data.Date, prev_date_str) != 0) {
            // 清除旧的日期区域
            draw_filled_rect(SCREEN_WIDTH - 120, STATUS_Y + 25, 120, 12, COLOR_BLACK);
            // 绘制完整的日期
            snprintf(buffer, sizeof(buffer), "Date: %s", current_gps_data.Date);
            draw_string(SCREEN_WIDTH - 120, STATUS_Y + 25, buffer, COLOR_CYAN, COLOR_BLACK);
            strcpy(prev_date_str, current_gps_data.Date);
        }
    }
}

// 删除重复的draw_complete_interface函数定义

/**
 * @brief 更新显示内容（简化版本，参考vendor_gps_display.c）
 */
void update_display() {
    // 直接调用各个显示函数，它们内部会处理局部刷新
    draw_gps_coordinates(COORDINATES_Y, false);
    draw_satellite_signal_bars(SIGNAL_BARS_Y, false);
    draw_status_bar(false);
}

/**
 * @brief 完整界面绘制（仅在初始化时使用）
 */
void draw_complete_interface() {
    // 清屏
    fill_screen(COLOR_BLACK);
    
    // 绘制各个区域
    draw_header();
    draw_gps_coordinates(DATA_START_Y, true);  // 第一次绘制，清除区域
    draw_satellite_signal_bars(SIGNAL_BARS_Y, true);
    draw_status_bar(true);
}

// =============================================================================
// 主程序
// =============================================================================

/**
 * @brief GPS ILI9488显示示例主函数
 */
extern "C" int vendor_gps_ili9488_demo() {
    printf("\n=== LC76X GPS + ILI9488显示示例 ===\n");
    printf("编译时间: %s %s\n", __DATE__, __TIME__);
    printf("功能: GPS数据接收与ILI9488显示\n\n");
    
    // 设置调试模式
    vendor_gps_set_debug(enable_debug);
    printf("调试模式: %s\n", enable_debug ? "已启用" : "已禁用");
    
    // 记录系统启动时间
    system_start_time = to_ms_since_boot(get_absolute_time());
    
    // 初始化GPS模块
    printf("正在初始化GPS模块...\n");
    if (!vendor_gps_init(GPS_UART_ID, GPS_BAUD_RATE, GPS_TX_PIN, GPS_RX_PIN, GPS_FORCE_PIN)) {
        printf("错误：GPS模块初始化失败，请检查连接并重试\n");
        return -1;
    }
    
    printf("GPS初始化成功，开始接收数据\n");
    printf("UART%d引脚: TX=%d, RX=%d, 波特率=%d\n", 
           GPS_UART_ID, GPS_TX_PIN, GPS_RX_PIN, GPS_BAUD_RATE);
    
    // 发送设置命令 - 配置NMEA输出
    printf("发送NMEA输出配置命令...\n");
    
    // 启用RMC和GGA语句输出，提升输出频率
    vendor_gps_send_command("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    sleep_ms(100);
    
    // 设置更新频率为1Hz (1000ms)
    vendor_gps_send_command("$PMTK220,1000");
    sleep_ms(500);
    
    printf("注意: 程序已增强，现在能解析GNRMC/GPRMC或GNGGA/GPGGA语句\n");
    printf("      速度和航向信息来自RMC语句，坐标信息支持两种语句\n");
    
    // 设置随机数种子（用于卫星信号图模拟）
    srand(time_us_32());
    
    // 初始化显示驱动
    driver = new ILI9488Driver(ILI9488_GET_SPI_CONFIG());
    gfx = new PicoILI9488GFX<ILI9488Driver>(*driver, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    if (!driver->initialize()) {
        printf("错误：ILI9488驱动初始化失败\n");
        return -1;
    }
    
    // 设置屏幕参数
    driver->setRotation(Rotation::Portrait_180);  // 旋转180度
    driver->setBacklight(true);
    display_initialized = true;
    
    printf("系统初始化完成，开始运行...\n");
    
    // 显示启动画面
    fill_screen(COLOR_BLACK);
    draw_string(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 20, "GPS Position Monitor", COLOR_WHITE, COLOR_BLACK);
    draw_string(SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2, "Searching satellites...", COLOR_YELLOW, COLOR_BLACK);
    
    // 初始化GPS数据结构
    memset(&current_gps_data, 0, sizeof(GNRMC));
    memset(&previous_gps_data, 0, sizeof(GNRMC));
    
    // 立即尝试获取GPS数据
    printf("\n初始状态...\n");
    update_gps_data();
    if (!gps_data_updated) {
        printf("警告: 初始化未能获得有效GPS数据，请检查GPS模块连接\n");
    }
    
    // 绘制完整界面
    draw_complete_interface();
    
    // 主循环
    uint32_t loop_count = 0;
    uint32_t last_gps_update = 0;    // GPS数据更新计时器
    
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // 每秒更新GPS数据
        if (current_time - last_gps_update >= 1000) {
            // GPS数据处理
            update_gps_data();
            
            // 更新显示（局部刷新）
            if (display_initialized) {
                update_display();
            }
            
            last_gps_update = current_time;
        }
        
        // 性能统计
        loop_count++;
        if (loop_count % 1000 == 0) {
            printf("系统运行正常，循环计数: %lu，GPS数据包计数: %lu，有效定位次数: %lu\n", 
                   loop_count, packet_count, valid_fix_count);
            
            if (gps_data_updated) {
                printf("当前GPS状态: 纬度=%.6f, 经度=%.6f, 状态=%d, 时间=%02d:%02d:%02d\n",
                       current_gps_data.Lat, current_gps_data.Lon, 
                       current_gps_data.Status,
                       current_gps_data.Time_H, current_gps_data.Time_M, current_gps_data.Time_S);
            } else {
                printf("当前GPS状态: 无有效数据\n");
            }
        }
        
        // 短暂延时，避免CPU占用过高
        sleep_ms(10);
    }
    
    printf("程序退出\n");
    return 0;
}

/**
 * @brief 程序入口点
 */
extern "C" int main() {
    // 初始化Pico SDK
    stdio_init_all();
    
    // 等待USB连接（调试用）
    printf("等待系统就绪...\n");
    sleep_ms(2000);
    
    // 运行主程序
    return vendor_gps_ili9488_demo();
} 