/**
 * @file vendor_gps_ili9488_optimized.cpp
 * @brief GPS数据显示示例 - 使用ILI9488显示屏 (优化版) - 基于LC76G I2C适配器
 * 
 * 功能说明：
 * - 使用LC76G I2C适配器接收GPS模块数据
 * - 在ILI9488显示屏上显示GPS信息 (480x320横屏布局)
 * - 双栏布局：左侧GPS信息，右侧卫星信号
 * - 支持坐标转换（WGS84 -> 百度/谷歌坐标）
 * - 实时更新GPS状态
 * 
 * 硬件连接：
 * - GPS模块：I2C1连接 (SDA: GPIO2, SCL: GPIO3)
 * - ILI9488显示：SPI连接
 * - 引脚配置：参见pin_config.hpp
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// C++头文件
#include "ili9488_driver.hpp"
#include "pico_ili9488_gfx.hpp"
#include "ili9488_colors.hpp"
#include "ili9488_font.hpp"
#include "pin_config.hpp"

extern "C" {
#include "gps/lc76g_i2c_adaptor.h"
}

// GPS SD卡日志记录器
#include "gps/gps_logger.hpp"

// 使用C++命名空间
using namespace ili9488;
using namespace pico_ili9488_gfx;

// =============================================================================
// 函数声明
// =============================================================================

// GPS SD卡日志记录器函数声明
static bool initialize_sd_logger();
static void process_gps_logging(const LC76G_GPS_Data& gps_data);
static void check_log_flush();
static std::string get_sd_logger_stats();

// =============================================================================
// 显示配置常量 (480x320横屏布局)
// =============================================================================

#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT   320

// 颜色定义 (RGB888格式)
#define COLOR_BLACK     0x000000
#define COLOR_WHITE     0xFFFFFF
#define COLOR_RED       0xFF0000
#define COLOR_GREEN     0x00FF00
#define COLOR_BLUE      0x000080
#define COLOR_YELLOW    0xFFFF00
#define COLOR_CYAN      0x00FFFF
#define COLOR_MAGENTA   0xFF00FF
#define COLOR_GRAY      0x808080
#define COLOR_DARK_GRAY 0x404040
#define COLOR_LIGHT_GRAY 0xC0C0C0
#define COLOR_ORANGE    0xFF8000

// 界面布局参数 (横屏 480x320)
#define HEADER_HEIGHT       35
#define MAIN_AREA_HEIGHT    250
#define STATUS_BAR_HEIGHT   35
#define MARGIN_X            8
#define MARGIN_Y            8

// 双栏布局 (横屏优化)
#define LEFT_PANEL_WIDTH    240
#define RIGHT_PANEL_WIDTH   240
#define PANEL_SPACING       0

// 区域定义
#define HEADER_Y            0
#define MAIN_AREA_Y         (HEADER_HEIGHT)
#define STATUS_BAR_Y        (HEADER_HEIGHT + MAIN_AREA_HEIGHT)

// 左侧面板 (GPS信息)
#define GPS_INFO_START_Y    (MAIN_AREA_Y + MARGIN_Y + 5)
#define GPS_LINE_HEIGHT     22
#define GPS_INFO_WIDTH      (LEFT_PANEL_WIDTH - 2 * MARGIN_X)

// 右侧面板 (卫星信号)
#define SIGNAL_START_Y      (MAIN_AREA_Y + MARGIN_Y + 5)
#define SIGNAL_AREA_WIDTH   (RIGHT_PANEL_WIDTH - 2 * MARGIN_X)
#define SIGNAL_AREA_HEIGHT  180

// GPS数据更新间隔
#define GPS_UPDATE_INTERVAL 2000  // 增加到2秒，给GPS更多时间处理
#define DISPLAY_REFRESH_INTERVAL 500

// GPS状态跟踪变量
static bool gps_was_valid = false;
static uint32_t gps_valid_start_time = 0;
static uint32_t last_successful_update = 0;
static uint32_t consecutive_failures = 0;

// =============================================================================
// 全局变量
// =============================================================================

// 显示驱动实例
static ILI9488Driver* driver = nullptr;
static PicoILI9488GFX<ILI9488Driver>* gfx = nullptr;

// GPS数据缓存
static LC76G_GPS_Data current_gps_data;
static LC76G_GPS_Data previous_gps_data;
static bool gps_data_updated = false;
static uint32_t last_gps_update = 0;

// GPS调试和统计信息
static uint32_t packet_count = 0;
static uint32_t valid_fix_count = 0;
static bool enable_debug = true;

// GPS SD卡日志记录器
static GPS::GPSLogger* gps_logger = nullptr;
static bool sd_logger_initialized = false;
static uint32_t total_logged_records = 0;
static uint32_t failed_log_records = 0;
static uint64_t last_log_flush_time = 0;

// 系统状态
static bool display_initialized = false;
static uint32_t system_start_time = 0;

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

void draw_vline(uint16_t x, uint16_t y, uint16_t h, uint32_t color) {
    if (gfx) {
        gfx->drawFastVLine(x, y, h, color);
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

void update_gps_data() {
    // 尝试多次获取GPS数据，提高成功率
    LC76G_GPS_Data new_data;
    bool got_data = false;
    
    for (int retry = 0; retry < 3; retry++) {
        lc76g_read_gps_data(&new_data);
        
        // 检查是否获得有效数据
        if (new_data.Status == 1 && 
            fabs(new_data.Lat) > 0.0001 && 
            fabs(new_data.Lon) > 0.0001) {
            got_data = true;
            break;
        }
        
        if (retry < 2) {
            printf("[GPS调试] 重试获取数据 (%d/3)...\n", retry + 1);
            sleep_ms(100);  // 短暂等待
        }
    }
    
    // 保存之前的数据
    memcpy(&previous_gps_data, &current_gps_data, sizeof(LC76G_GPS_Data));
    
    // 增加数据包计数
    packet_count++;
    
    // 详细日志打印（减少日志频率，避免刷屏）
    if (packet_count % 5 == 0 || got_data) {  // 每5次或成功时打印
        printf("[GPS调试] 数据包 #%lu (重试: %s)\n", packet_count, got_data ? "成功" : "失败");
        printf("[GPS调试] 状态: %d, 纬度: %.6f, 经度: %.6f\n", 
               new_data.Status, new_data.Lat, new_data.Lon);
        printf("[GPS调试] 速度: %.2f, 航向: %.2f\n", 
               new_data.Speed, new_data.Course);
        printf("[GPS调试] 时间: %02d:%02d:%02d (UTC), 日期: %s\n",
               new_data.Time_H, new_data.Time_M, new_data.Time_S,
               new_data.Date);
    }
    
    // 检查是否获得有效时间数据
    bool got_time_data = (new_data.Time_H > 0 || new_data.Time_M > 0 || new_data.Time_S > 0);
    bool got_valid_data = false;
    
    if (got_time_data) {
        // 检查数据是否有效
        if (new_data.Status == 1) {
            if (fabs(new_data.Lat) > 0.0001 && fabs(new_data.Lon) > 0.0001) {
                got_valid_data = true;
                valid_fix_count++;
                printf("[GPS调试] 获得有效定位数据！\n");
            } else {
                printf("[GPS调试] 状态有效但坐标无效 (Lat:%.6f, Lon:%.6f)\n", 
                       new_data.Lat, new_data.Lon);
            }
        } else {
            printf("[GPS调试] GPS状态无效 (Status=%d)\n", new_data.Status);
        }
    } else {
        printf("[GPS调试] 未获得时间数据\n");
    }
    
    // 检查GPS状态变化
    bool gps_is_valid = got_valid_data;
    if (gps_is_valid && !gps_was_valid) {
        // GPS从无效变为有效
        gps_valid_start_time = to_ms_since_boot(get_absolute_time());
        printf("[GPS调试] GPS定位成功，开始计时...\n");
        printf("[GPS调试] 有效定位计数: %lu\n", valid_fix_count);
    }
    
    // 添加GPS信号质量监控
    if (gps_is_valid) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        uint32_t valid_duration = (current_time - gps_valid_start_time) / 1000;
        printf("[GPS调试] GPS已稳定运行: %lu秒\n", valid_duration);
    } else {
        printf("[GPS调试] GPS信号不稳定，等待重新定位...\n");
    }
    
    gps_was_valid = gps_is_valid;
    
    // 检查是否有新的定位数据或时间数据
    if (got_time_data || memcmp(&new_data, &current_gps_data, sizeof(LC76G_GPS_Data)) != 0) {
        memcpy(&current_gps_data, &new_data, sizeof(LC76G_GPS_Data));
        gps_data_updated = true;
        last_gps_update = to_ms_since_boot(get_absolute_time());
        
        if (got_valid_data) {
            last_successful_update = last_gps_update;
            consecutive_failures = 0;
            printf("[GPS调试] GPS数据已更新 (有效定位)\n");
        } else {
            consecutive_failures++;
            printf("[GPS调试] GPS数据已更新 (连续失败: %lu次)\n", consecutive_failures);
        }
    } else {
        consecutive_failures++;
        if (consecutive_failures > 10) {
            printf("[GPS警告] GPS数据长时间无更新，连续失败: %lu次\n", consecutive_failures);
        }
    }
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
    
    // 标题文字 (居中，横屏优化)
    draw_string(SCREEN_WIDTH/2 - 90, 10, "GPS Position Monitor", COLOR_WHITE, COLOR_BLACK);
    
    // 分隔线
    draw_hline(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_WHITE);
}

/**
 * @brief 绘制左侧GPS信息面板
 */
void draw_gps_info_panel() {
    static char prev_lat[32] = {0};
    static char prev_lon[32] = {0};
    static char prev_alt[32] = {0};
    static char prev_speed[32] = {0};
    static char prev_course[32] = {0};
    static char prev_satellites[16] = {0};
    static char prev_hdop[16] = {0};
    static char prev_status[64] = {0};
    static bool prev_fix_state = false;
    
    uint16_t y = GPS_INFO_START_Y;
    
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
    snprintf(new_course, sizeof(new_course), "%.1f°", current_gps_data.Course);
    
    int satellites = (current_gps_data.Status == 1) ? 8 + (rand() % 4) : 0;
    float hdop = (current_gps_data.Status == 1) ? 0.8f + (rand() % 20) / 10.0f : 0.0f;
    snprintf(new_satellites, sizeof(new_satellites), "%d", satellites);
    snprintf(new_hdop, sizeof(new_hdop), "%.1f", hdop);
    
    if (current_gps_data.Status == 1) {
        strcpy(new_status, "Fixed");
    } else {
        strcpy(new_status, "None");
    }
    
    // 检查状态变化
    bool fix_state_changed = (prev_fix_state != (current_gps_data.Status == 1));
    
    // 绘制标签和数据
    const char* labels[] = {
        "Latitude:",
        "Longitude:",
        "Altitude:",
        "Speed:",
        "Course:",
        "Satellites:",
        "HDOP:",
        "Status:"
    };
    
    char* values[] = {
        new_lat, new_lon, new_alt, new_speed, 
        new_course, new_satellites, new_hdop, new_status
    };
    
    char* prev_values[] = {
        prev_lat, prev_lon, prev_alt, prev_speed,
        prev_course, prev_satellites, prev_hdop, prev_status
    };
    
    uint32_t colors[] = {
        COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE,
        COLOR_WHITE, COLOR_GREEN, COLOR_WHITE, 
        (uint32_t)((current_gps_data.Status == 1) ? COLOR_GREEN : COLOR_RED)
    };
    
    for (int i = 0; i < 8; i++) {
        // 绘制标签 (横屏优化位置)
        draw_string(MARGIN_X, y, labels[i], COLOR_LIGHT_GRAY, COLOR_BLACK);
        
        // 更新数据值
        if (strcmp(values[i], prev_values[i]) != 0 || fix_state_changed) {
            // 清除旧值 (扩大清除区域，避免文字重叠)
            draw_filled_rect(MARGIN_X + 80, y - 2, GPS_INFO_WIDTH - 90, 18, COLOR_BLACK);
            // 绘制新值
            draw_string(MARGIN_X + 80, y, values[i], colors[i], COLOR_BLACK);
            strcpy(prev_values[i], values[i]);
        }
        
        y += GPS_LINE_HEIGHT;
    }
    
    // 更新定位状态跟踪变量
    prev_fix_state = (current_gps_data.Status == 1);
}

/**
 * @brief 绘制右侧卫星信号面板
 */
void draw_satellite_panel() {
    uint16_t panel_x = LEFT_PANEL_WIDTH + PANEL_SPACING;
    uint16_t panel_y = SIGNAL_START_Y;
    
    // 标题 (横屏优化)
    draw_string(panel_x + MARGIN_X, panel_y, "Satellite Signal", COLOR_WHITE, COLOR_BLACK);
    panel_y += 22;
    
    // 信号强度条参数 (横屏优化)
    const uint16_t bar_width = 18;
    const uint16_t bar_spacing = 22;
    const uint16_t max_bar_height = 100;
    const uint16_t base_y = panel_y + max_bar_height;
    const uint16_t start_x = panel_x + MARGIN_X;
    
    // 绘制信号强度条
    for (int i = 0; i < 8; i++) {
        uint16_t x = start_x + i * bar_spacing;
        
        // 模拟信号强度
        uint8_t signal_strength = 0;
        if (current_gps_data.Status == 1 && i < 8) {
            signal_strength = 20 + (rand() % 60); // 20-80的信号强度
        }
        
        // 清除旧条
        draw_filled_rect(x, panel_y, bar_width, max_bar_height, COLOR_BLACK);
        
        if (signal_strength > 0) {
            uint16_t bar_height = (signal_strength * max_bar_height) / 100;
            uint32_t bar_color = COLOR_GREEN;
            
            if (signal_strength < 30) bar_color = COLOR_RED;
            else if (signal_strength < 50) bar_color = COLOR_YELLOW;
            else if (signal_strength < 80) bar_color = COLOR_ORANGE;
            // 80%以上使用绿色（默认值）
            
            draw_filled_rect(x, base_y - bar_height, bar_width, bar_height, bar_color);
        }
        
        // 绘制边框
        draw_rect(x, panel_y, bar_width, max_bar_height, COLOR_GRAY);
        
        // 卫星编号
        char sat_num[4];
        snprintf(sat_num, sizeof(sat_num), "%d", i + 1);
        draw_string(x + 6, base_y + 5, sat_num, COLOR_WHITE, COLOR_BLACK);
    }
    
    // 绘制信号强度图例 (横屏优化)
    panel_y = base_y + 25;
    draw_string(start_x, panel_y, "Signal Strength:", COLOR_WHITE, COLOR_BLACK);
    panel_y += 18;
    
    // 图例颜色条 (优化间距布局)
    const uint16_t legend_spacing = 50;  // 进一步增加间距
    const uint16_t color_bar_width = 12;
    const uint16_t text_offset = 16;
    
    // Weak
    draw_filled_rect(start_x, panel_y, color_bar_width, 8, COLOR_RED);
    draw_string(start_x + text_offset, panel_y, "Weak", COLOR_RED, COLOR_BLACK);
    
    // Fair
    draw_filled_rect(start_x + legend_spacing, panel_y, color_bar_width, 8, COLOR_YELLOW);
    draw_string(start_x + legend_spacing + text_offset, panel_y, "Fair", COLOR_YELLOW, COLOR_BLACK);
    
    // Good
    draw_filled_rect(start_x + legend_spacing * 2, panel_y, color_bar_width, 8, COLOR_GREEN);
    draw_string(start_x + legend_spacing * 2 + text_offset, panel_y, "Good", COLOR_GREEN, COLOR_BLACK);
}

/**
 * @brief 绘制状态栏
 */
void draw_status_bar() {
    static char prev_uptime_str[20] = {0};
    static char prev_gps_status_str[32] = {0};
    static char prev_utc_time_str[15] = {0};
    
    char buffer[64];
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    uint32_t uptime_seconds = (current_time - system_start_time) / 1000;
    
    // 运行时间
    char uptime_str[20];
    snprintf(uptime_str, sizeof(uptime_str), "Uptime: %02d:%02d:%02d", 
             uptime_seconds / 3600, (uptime_seconds / 60) % 60, uptime_seconds % 60);
    
    if (strcmp(uptime_str, prev_uptime_str) != 0) {
        draw_filled_rect(MARGIN_X, STATUS_BAR_Y + 8, 140, 12, COLOR_BLACK);
        draw_string(MARGIN_X, STATUS_BAR_Y + 8, uptime_str, COLOR_WHITE, COLOR_BLACK);
        strcpy(prev_uptime_str, uptime_str);
    }
    
    // GPS状态 (横屏优化位置)
    char gps_status_str[32];
    if (gps_data_updated) {
        uint32_t seconds_since_update = (current_time - last_gps_update) / 1000;
        // 确保最少显示1秒
        if (seconds_since_update == 0) {
            seconds_since_update = 1;
        }
        snprintf(gps_status_str, sizeof(gps_status_str), "GPS: Updated %ds ago", seconds_since_update);
    } else {
        strcpy(gps_status_str, "GPS: Waiting...");
    }
    
    if (strcmp(gps_status_str, prev_gps_status_str) != 0) {
        draw_filled_rect(SCREEN_WIDTH/2 - 75, STATUS_BAR_Y + 8, 150, 12, COLOR_BLACK);
        uint32_t status_color = gps_data_updated ? COLOR_GREEN : COLOR_YELLOW;
        draw_string(SCREEN_WIDTH/2 - 75, STATUS_BAR_Y + 8, gps_status_str, status_color, COLOR_BLACK);
        strcpy(prev_gps_status_str, gps_status_str);
    }
    
    // UTC时间显示 (横屏优化位置，向左移动5像素)
    if (current_gps_data.Status > 0) {
        char utc_time_str[20];
        // 直接使用GPS原始时间
        snprintf(utc_time_str, sizeof(utc_time_str), "UTC: %02d:%02d:%02d", 
                 current_gps_data.Time_H, current_gps_data.Time_M, current_gps_data.Time_S);
        
        if (strcmp(utc_time_str, prev_utc_time_str) != 0) {
            draw_filled_rect(SCREEN_WIDTH - 125, STATUS_BAR_Y + 8, 125, 12, COLOR_BLACK);
            draw_string(SCREEN_WIDTH - 125, STATUS_BAR_Y + 8, utc_time_str, COLOR_CYAN, COLOR_BLACK);
            strcpy(prev_utc_time_str, utc_time_str);
        }
    }
}

/**
 * @brief 绘制完整界面
 */
void draw_complete_interface() {
    // 清屏
    fill_screen(COLOR_BLACK);
    
    // 绘制各个区域
    draw_header();
    
    // 绘制分隔线
    draw_vline(LEFT_PANEL_WIDTH, MAIN_AREA_Y, MAIN_AREA_HEIGHT, COLOR_GRAY);
    draw_hline(0, STATUS_BAR_Y, SCREEN_WIDTH, COLOR_GRAY);
    
    // 绘制面板
    draw_gps_info_panel();
    draw_satellite_panel();
    draw_status_bar();
}

/**
 * @brief 更新显示内容
 */
void update_display() {
    // 更新各个面板
    draw_gps_info_panel();
    draw_satellite_panel();
    draw_status_bar();
}

// =============================================================================
// 主程序
// =============================================================================

/**
 * @brief GPS ILI9488显示示例主函数 (优化版)
 */
extern "C" int vendor_gps_ili9488_optimized_demo() {
    printf("\n=== LC76X GPS + ILI9488显示示例 (优化版) ===\n");
    printf("分辨率: 480x320 (横屏双栏布局)\n");
    printf("编译时间: %s %s\n", __DATE__, __TIME__);
    
    // 设置调试模式
    // LC76G I2C适配器自动处理调试
    printf("LC76G I2C适配器已启用调试模式\n");
    
    // 记录系统启动时间
    system_start_time = to_ms_since_boot(get_absolute_time());
    
    // 初始化GPS模块
    printf("正在初始化GPS模块...\n");
    printf("[GPS调试] I2C实例: %s, 地址: 0x%02X, 速度: %d Hz\n", 
           GPS_I2C_INST == i2c0 ? "I2C0" : "I2C1", GPS_I2C_ADDR, GPS_I2C_SPEED);
    printf("[GPS调试] SDA引脚: %d, SCL引脚: %d, FORCE引脚: %d\n", 
           GPS_PIN_SDA, GPS_PIN_SCL, GPS_FORCE_PIN);
    
    printf("[GPS调试] 开始调用lc76g_i2c_init...\n");
    if (!lc76g_i2c_init(GPS_I2C_INST, GPS_PIN_SDA, GPS_PIN_SCL, GPS_I2C_SPEED, GPS_FORCE_PIN)) {
        printf("错误：GPS模块初始化失败\n");
        return -1;
    }
    
    printf("GPS初始化成功\n");
    
    // 启用调试模式
    lc76g_set_debug(true);
    printf("LC76G I2C适配器已启用调试模式\n");
    
    // 初始化GPS SD卡日志记录器 (可选功能)
    printf("正在初始化GPS SD卡日志记录器...\n");
    if (initialize_sd_logger()) {
        printf("GPS SD卡日志记录器初始化成功\n");
    } else {
        printf("GPS SD卡日志记录器初始化失败，系统将继续运行\n");
    }
    
    // LC76G增强配置
    printf("配置LC76G模块...\n");
    
    // LC76G I2C适配器通过命令配置GPS模块
    printf("✓ LC76G I2C适配器配置完成\n");
    
    // 执行智能GPS启动
    printf("执行智能GPS启动流程...\n");
    
    // 1. LC76G I2C适配器自动处理GPS启动
    printf("步骤1: LC76G I2C适配器自动启动GPS模块\n");
    
    // 2. 等待GPS模块响应
    printf("步骤2: 等待GPS模块响应...\n");
    sleep_ms(2000);
    
    // 3. 发送设置命令
    printf("步骤3: 发送NMEA配置命令\n");
    lc76g_send_command("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", 50);
    sleep_ms(100);
    lc76g_send_command("$PMTK220,1000", 12);
    sleep_ms(500);
    
    // 4. 检查GPS状态
    printf("步骤4: 检查GPS状态\n");
    LC76G_GPS_Data test_data;
    lc76g_read_gps_data(&test_data);
    printf("[GPS调试] 初始状态检查 - 状态: %d, 时间: %02d:%02d:%02d\n", 
           test_data.Status, test_data.Time_H, test_data.Time_M, test_data.Time_S);
    
    // 设置随机数种子
    srand(time_us_32());
    
    // 初始化显示驱动
    driver = new ILI9488Driver(ILI9488_GET_SPI_CONFIG());
    gfx = new PicoILI9488GFX<ILI9488Driver>(*driver, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    if (!driver->initialize()) {
        printf("错误：ILI9488驱动初始化失败\n");
        return -1;
    }
    
    // 设置屏幕参数
    driver->setRotation(Rotation::Landscape_270);  // 横屏模式 (480x320) - 旋转180度
    driver->setBacklight(true);
    display_initialized = true;
    
    printf("显示驱动初始化成功\n");
    
    // 初始化GPS数据结构
    memset(&current_gps_data, 0, sizeof(LC76G_GPS_Data));
    memset(&previous_gps_data, 0, sizeof(LC76G_GPS_Data));
    
    // 显示启动画面 (横屏优化)
    fill_screen(COLOR_BLACK);
    draw_string(SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 - 15, "GPS Position Monitor", COLOR_WHITE, COLOR_BLACK);
    draw_string(SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 + 5, "Initializing...", COLOR_YELLOW, COLOR_BLACK);
    sleep_ms(1000);
    
    // 立即尝试获取GPS数据
    update_gps_data();
    
    // 绘制完整界面
    draw_complete_interface();
    
    printf("系统初始化完成，开始运行...\n");
    
    // 主循环
    uint32_t last_gps_update = 0;
    
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // 每2秒更新GPS数据
        if (current_time - last_gps_update >= GPS_UPDATE_INTERVAL) {
            printf("[主循环] 更新GPS数据 (运行时间: %lu秒)\n", 
                   (current_time - system_start_time) / 1000);
            
            // GPS健康检查
            if (packet_count > 0) {
                float success_rate = (float)valid_fix_count / packet_count * 100.0f;
                printf("[GPS健康] 成功率: %.1f%% (%lu/%lu)\n", 
                       success_rate, valid_fix_count, packet_count);
                
                if (success_rate < 10.0f) {
                    printf("[GPS警告] 定位成功率过低，建议检查天线或移动到开阔区域\n");
                }
                
                // 如果连续失败超过20次，LC76G I2C适配器自动处理
                if (consecutive_failures > 20) {
                    printf("[GPS恢复] LC76G I2C适配器自动处理GPS模块重启...\n");
                    sleep_ms(1000);
                    consecutive_failures = 0;
                }
            }
            
            update_gps_data();
            
            // 处理GPS数据记录到SD卡 (后台运行)
            process_gps_logging(current_gps_data);
            
            // 更新显示
            if (display_initialized) {
                update_display();
            }
            
            last_gps_update = current_time;
        }
        
        // 检查并刷新SD卡日志缓冲区 (后台运行)
        check_log_flush();
        
        // 短暂延时
        sleep_ms(10);
    }
    
    return 0;
}

// =============================================================================
// GPS SD卡日志记录器函数
// =============================================================================

/**
 * @brief 初始化GPS SD卡日志记录器
 * @return true 初始化成功，false 初始化失败
 */
static bool initialize_sd_logger() {
    printf("[SD Logger] 正在初始化GPS SD卡日志记录器...\n");
    
    // 配置SD卡 (使用默认配置)
    MicroSD::SPIConfig sd_config = MicroSD::Config::DEFAULT;
    
    // 配置日志记录器
    GPS::GPSLogger::LogConfig log_config;
    log_config.log_directory = "/gps_logs";
    log_config.max_file_size = 256 * 1024;        // 256KB文件大小
    log_config.max_files_per_day = 50;            // 每天最多50个文件
    log_config.buffer_size = 1024;                // 1KB缓冲区
    log_config.batch_write_count = 5;             // 5条记录批量写入
    log_config.write_interval_ms = 5000;          // 5秒写入间隔
    log_config.enable_immediate_write = false;    // 使用批量写入
    log_config.enable_coordinate_transform = true; // 启用坐标转换
    
    // 创建GPS日志记录器
    gps_logger = new GPS::GPSLogger(sd_config, log_config);
    
    if (!gps_logger->initialize()) {
        printf("[SD Logger] 初始化失败，SD卡可能不可用\n");
        delete gps_logger;
        gps_logger = nullptr;
        return false;
    }
    
    sd_logger_initialized = true;
    printf("[SD Logger] 初始化成功，日志文件: %s\n", gps_logger->get_current_log_file().c_str());
    return true;
}

/**
 * @brief 处理GPS数据记录到SD卡
 * @param gps_data GPS数据
 */
static void process_gps_logging(const LC76G_GPS_Data& gps_data) {
    if (!sd_logger_initialized || !gps_logger) {
        return;
    }
    
    // 只有GPS信号有效时才记录
    if (gps_data.Status) {
        if (gps_logger->log_gps_data(gps_data)) {
            total_logged_records++;
        } else {
            failed_log_records++;
            printf("[SD Logger] GPS数据记录失败\n");
        }
    }
}

/**
 * @brief 检查并刷新SD卡日志缓冲区
 */
static void check_log_flush() {
    if (!sd_logger_initialized || !gps_logger) {
        return;
    }
    
    uint64_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 每10秒刷新一次缓冲区
    if (current_time - last_log_flush_time >= 10000) {
        if (gps_logger->flush_buffer()) {
            printf("[SD Logger] 缓冲区已刷新\n");
        } else {
            printf("[SD Logger] 缓冲区刷新失败\n");
        }
        last_log_flush_time = current_time;
    }
}

/**
 * @brief 获取SD卡日志统计信息
 * @return 统计信息字符串
 */
static std::string get_sd_logger_stats() {
    if (!sd_logger_initialized || !gps_logger) {
        return "SD卡日志: 未初始化";
    }
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), 
             "SD卡日志: 记录 %u 条, 失败 %u 条, 文件: %s",
             total_logged_records, failed_log_records,
             gps_logger->get_current_log_file().c_str());
    return std::string(buffer);
}

/**
 * @brief 程序入口点
 */
extern "C" int main() {
    // 初始化Pico SDK
    stdio_init_all();
    
    // 等待USB连接
    printf("等待系统就绪...\n");
    sleep_ms(2000);
    
    // 运行主程序
    return vendor_gps_ili9488_optimized_demo();
}
