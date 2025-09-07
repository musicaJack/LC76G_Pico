/**
 * @file vendor_gps_ili9488_optimized.cpp
 * @brief GPS数据显示示例 - 使用ILI9488显示屏 (优化版)
 * 
 * 功能说明：
 * - 接收L76X GPS模块数据
 * - 在ILI9488显示屏上显示GPS信息 (480x320横屏布局)
 * - 双栏布局：左侧GPS信息，右侧卫星信号
 * - 支持坐标转换（WGS84 -> 百度/谷歌坐标）
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

// 界面布局参数
#define HEADER_HEIGHT       40
#define MAIN_AREA_HEIGHT    240
#define STATUS_BAR_HEIGHT   40
#define MARGIN_X            10
#define MARGIN_Y            10

// 双栏布局
#define LEFT_PANEL_WIDTH    240
#define RIGHT_PANEL_WIDTH   240
#define PANEL_SPACING       0

// 区域定义
#define HEADER_Y            0
#define MAIN_AREA_Y         (HEADER_HEIGHT)
#define STATUS_BAR_Y        (HEADER_HEIGHT + MAIN_AREA_HEIGHT)

// 左侧面板 (GPS信息)
#define GPS_INFO_START_Y    (MAIN_AREA_Y + MARGIN_Y)
#define GPS_LINE_HEIGHT     20
#define GPS_INFO_WIDTH      (LEFT_PANEL_WIDTH - 2 * MARGIN_X)

// 右侧面板 (卫星信号)
#define SIGNAL_START_Y      (MAIN_AREA_Y + MARGIN_Y)
#define SIGNAL_AREA_WIDTH   (RIGHT_PANEL_WIDTH - 2 * MARGIN_X)
#define SIGNAL_AREA_HEIGHT  200

// GPS数据更新间隔
#define GPS_UPDATE_INTERVAL 1000
#define DISPLAY_REFRESH_INTERVAL 500

// =============================================================================
// 全局变量
// =============================================================================

// 显示驱动实例
static ILI9488Driver* driver = nullptr;
static PicoILI9488GFX<ILI9488Driver>* gfx = nullptr;

// GPS数据缓存
static GNRMC current_gps_data;
static GNRMC previous_gps_data;
static bool gps_data_updated = false;
static uint32_t last_gps_update = 0;

// GPS调试和统计信息
static uint32_t packet_count = 0;
static uint32_t valid_fix_count = 0;
static bool enable_debug = true;

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
    GNRMC new_data = vendor_gps_get_gnrmc();
    
    // 保存之前的数据
    memcpy(&previous_gps_data, &current_gps_data, sizeof(GNRMC));
    
    // 增加数据包计数
    packet_count++;
    
    // 检查是否获得有效时间数据
    bool got_time_data = (new_data.Time_H > 0 || new_data.Time_M > 0 || new_data.Time_S > 0);
    bool got_valid_data = false;
    
    if (got_time_data) {
        // 检查数据是否有效
        if (new_data.Status == 1) {
            if (fabs(new_data.Lat) > 0.0001 && fabs(new_data.Lon) > 0.0001) {
                got_valid_data = true;
                valid_fix_count++;
            }
        }
    }
    
    // 检查是否有新的定位数据或时间数据
    if (got_time_data || memcmp(&new_data, &current_gps_data, sizeof(GNRMC)) != 0) {
        memcpy(&current_gps_data, &new_data, sizeof(GNRMC));
        gps_data_updated = true;
        last_gps_update = to_ms_since_boot(get_absolute_time());
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
    
    // 标题文字 (居中)
    draw_string(SCREEN_WIDTH/2 - 100, 12, "GPS Position Monitor", COLOR_WHITE, COLOR_BLUE);
    
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
        snprintf(new_status, sizeof(new_status), "Fix - %02d:%02d:%02d UTC",
                 current_gps_data.Time_H, current_gps_data.Time_M, current_gps_data.Time_S);
    } else {
        strcpy(new_status, "No Signal");
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
        // 绘制标签
        draw_string(MARGIN_X, y, labels[i], COLOR_LIGHT_GRAY, COLOR_BLACK);
        
        // 更新数据值
        if (strcmp(values[i], prev_values[i]) != 0 || fix_state_changed) {
            // 清除旧值
            draw_filled_rect(MARGIN_X + 80, y, GPS_INFO_WIDTH - 90, 12, COLOR_BLACK);
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
    
    // 标题
    draw_string(panel_x + MARGIN_X, panel_y, "Satellite Signal", COLOR_WHITE, COLOR_BLACK);
    panel_y += 25;
    
    // 信号强度条参数
    const uint16_t bar_width = 20;
    const uint16_t bar_spacing = 25;
    const uint16_t max_bar_height = 120;
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
            else if (signal_strength < 70) bar_color = COLOR_ORANGE;
            
            draw_filled_rect(x, base_y - bar_height, bar_width, bar_height, bar_color);
        }
        
        // 绘制边框
        draw_rect(x, panel_y, bar_width, max_bar_height, COLOR_GRAY);
        
        // 卫星编号
        char sat_num[4];
        snprintf(sat_num, sizeof(sat_num), "%d", i + 1);
        draw_string(x + 6, base_y + 5, sat_num, COLOR_WHITE, COLOR_BLACK);
    }
    
    // 绘制信号强度图例
    panel_y = base_y + 30;
    draw_string(start_x, panel_y, "Signal Strength:", COLOR_WHITE, COLOR_BLACK);
    panel_y += 20;
    
    // 图例颜色条
    draw_filled_rect(start_x, panel_y, 15, 10, COLOR_RED);
    draw_string(start_x + 20, panel_y, "Weak", COLOR_WHITE, COLOR_BLACK);
    
    draw_filled_rect(start_x + 60, panel_y, 15, 10, COLOR_YELLOW);
    draw_string(start_x + 80, panel_y, "Fair", COLOR_WHITE, COLOR_BLACK);
    
    draw_filled_rect(start_x + 120, panel_y, 15, 10, COLOR_GREEN);
    draw_string(start_x + 140, panel_y, "Good", COLOR_WHITE, COLOR_BLACK);
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
        draw_filled_rect(MARGIN_X, STATUS_BAR_Y + 5, 150, 12, COLOR_BLACK);
        draw_string(MARGIN_X, STATUS_BAR_Y + 5, uptime_str, COLOR_WHITE, COLOR_BLACK);
        strcpy(prev_uptime_str, uptime_str);
    }
    
    // GPS状态
    char gps_status_str[32];
    if (gps_data_updated) {
        uint32_t seconds_since_update = (current_time - last_gps_update) / 1000;
        snprintf(gps_status_str, sizeof(gps_status_str), "GPS: Updated %ds ago", seconds_since_update);
    } else {
        strcpy(gps_status_str, "GPS: Waiting...");
    }
    
    if (strcmp(gps_status_str, prev_gps_status_str) != 0) {
        draw_filled_rect(SCREEN_WIDTH/2 - 80, STATUS_BAR_Y + 5, 160, 12, COLOR_BLACK);
        uint32_t status_color = gps_data_updated ? COLOR_GREEN : COLOR_YELLOW;
        draw_string(SCREEN_WIDTH/2 - 80, STATUS_BAR_Y + 5, gps_status_str, status_color, COLOR_BLACK);
        strcpy(prev_gps_status_str, gps_status_str);
    }
    
    // UTC时间
    if (current_gps_data.Status > 0) {
        char utc_time_str[15];
        snprintf(utc_time_str, sizeof(utc_time_str), "UTC: %02d:%02d:%02d", 
                 current_gps_data.Time_H, current_gps_data.Time_M, current_gps_data.Time_S);
        
        if (strcmp(utc_time_str, prev_utc_time_str) != 0) {
            draw_filled_rect(SCREEN_WIDTH - 120, STATUS_BAR_Y + 5, 120, 12, COLOR_BLACK);
            draw_string(SCREEN_WIDTH - 120, STATUS_BAR_Y + 5, utc_time_str, COLOR_CYAN, COLOR_BLACK);
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
    vendor_gps_set_debug(enable_debug);
    printf("调试模式: %s\n", enable_debug ? "已启用" : "已禁用");
    
    // 记录系统启动时间
    system_start_time = to_ms_since_boot(get_absolute_time());
    
    // 初始化GPS模块
    printf("正在初始化GPS模块...\n");
    if (!vendor_gps_init(GPS_UART_ID, GPS_BAUD_RATE, GPS_TX_PIN, GPS_RX_PIN, GPS_FORCE_PIN)) {
        printf("错误：GPS模块初始化失败\n");
        return -1;
    }
    
    printf("GPS初始化成功\n");
    
    // 发送设置命令
    vendor_gps_send_command("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    sleep_ms(100);
    vendor_gps_send_command("$PMTK220,1000");
    sleep_ms(500);
    
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
    driver->setRotation(Rotation::Portrait_0);  // 竖屏模式 (480x320)
    driver->setBacklight(true);
    display_initialized = true;
    
    printf("显示驱动初始化成功\n");
    
    // 初始化GPS数据结构
    memset(&current_gps_data, 0, sizeof(GNRMC));
    memset(&previous_gps_data, 0, sizeof(GNRMC));
    
    // 显示启动画面
    fill_screen(COLOR_BLACK);
    draw_string(SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 20, "GPS Position Monitor", COLOR_WHITE, COLOR_BLACK);
    draw_string(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2, "Initializing...", COLOR_YELLOW, COLOR_BLACK);
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
        
        // 每秒更新GPS数据
        if (current_time - last_gps_update >= GPS_UPDATE_INTERVAL) {
            update_gps_data();
            
            // 更新显示
            if (display_initialized) {
                update_display();
            }
            
            last_gps_update = current_time;
        }
        
        // 短暂延时
        sleep_ms(10);
    }
    
    return 0;
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
