/**
 * @file vendor_gps_dashboard.cpp
 * @brief GPS仪表盘显示示例 - 高科技风格
 * 
 * 功能说明：
 * - 接收L76X GPS模块数据
 * - 初始显示GPS信息1分钟
 * - 自动切换到高科技仪表盘界面
 * - 包含罗盘和速度表
 * - 实时显示时间和状态信息
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
#include "hardware/gpio.h"

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


// 颜色定义 (RGB888格式)
#define COLOR_BLACK         0x000000
#define COLOR_WHITE         0xFFFFFF
#define COLOR_RED           0xFF0000
#define COLOR_GREEN         0x00FF00
#define COLOR_BLUE          0x000080
#define COLOR_CYAN          0x00FFFF
#define COLOR_YELLOW        0xFFFF00
#define COLOR_MAGENTA       0xFF00FF
#define COLOR_GRAY          0x808080
#define COLOR_DARK_BLUE     0x001122
#define COLOR_MEDIUM_BLUE   0x003366
#define COLOR_BRIGHT_BLUE   0x0088FF
#define COLOR_ORANGE        0xFF8000

// 军事级仪表盘布局参数 - 横屏480x320分辨率，仪表盘放大1.5倍
#define SCREEN_WIDTH        480         // 横屏宽度
#define SCREEN_HEIGHT       320         // 横屏高度
#define DIAL_RADIUS         120         // 仪表半径（放大1.5倍：80*1.5=120）
#define DIAL_SPACING        40          // 两个仪表盘之间的间距（调整以适应大仪表盘）
#define LEFT_DIAL_CENTER_X  120         // 左仪表中心X（1/4屏幕位置）
#define RIGHT_DIAL_CENTER_X 360         // 右仪表中心X（3/4屏幕位置）
#define DIAL_CENTER_Y       140         // 仪表中心Y（为底部状态栏留出空间）
// TIME_X 和 TIME_Y 已移除，数字时间显示已删除
#define SPEED_X             240         // 速度显示X（屏幕中心）
#define SPEED_Y             300         // 速度显示Y（时间下方）
#define STATUS_BAR_HEIGHT   40
#define TOP_INFO_HEIGHT     30          // 顶部GPS信息区域高度

// 状态定义
enum class DisplayState {
    DASHBOARD      // 显示仪表盘
};

// =============================================================================
// 全局变量
// =============================================================================

// 显示驱动实例
static ILI9488Driver* driver = nullptr;
static PicoILI9488GFX<ILI9488Driver>* gfx = nullptr;

// GPS数据缓存
static GNRMC current_gps_data;
static bool gps_data_updated = false;
static uint32_t last_gps_update = 0;

// 按键控制变量
static bool button_pressed = false;
static uint32_t button_press_start_time = 0;
static bool screen_on = true;
static bool button_processed = false;

// 局部更新标志
static bool need_update_compass = false;
static bool need_update_speedometer = false;
// need_update_time 已移除，数字时间显示使用内部局部刷新
static bool need_update_status = false;
static bool need_update_gps_signal = false;

// 时钟局部刷新相关变量
static uint8_t last_clock_hour = 0;
static uint8_t last_clock_minute = 0;
static uint8_t last_clock_second = 0;
static bool clock_initialized = false;

// 数字时间显示相关变量已移除

// GPS信号模拟数据（实际应用中应该从GPS模块获取）
static uint8_t gps_satellites_count = 0;
static uint8_t gps_signal_strength = 0;  // 0-100%
static uint8_t gps_satellites_in_view = 0;

// 显示状态
static DisplayState current_state = DisplayState::DASHBOARD;
static uint32_t gps_valid_start_time = 0;
static bool gps_was_valid = false;
static uint32_t system_start_time = 0;

// GPS统计信息
static uint32_t packet_count = 0;
static uint32_t valid_fix_count = 0;
static bool enable_debug = true;

// =============================================================================
// 按键处理函数
// =============================================================================

/**
 * @brief 检测按键状态并处理按键事件
 * @return true 如果有按键事件需要处理
 */
bool check_button_events() {
    static uint32_t last_button_check = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 防抖处理
    if (current_time - last_button_check < BUTTON_DEBOUNCE_MS) {
        return false;
    }
    last_button_check = current_time;
    
    bool button_state = !gpio_get(BUTTON_PIN);  // 按键按下时为低电平
    
    if (button_state && !button_pressed) {
        // 按键刚按下
        button_pressed = true;
        button_press_start_time = current_time;
        button_processed = false;
        printf("[按键] 按键按下\n");
        return false;
    }
    else if (!button_state && button_pressed) {
        // 按键释放
        button_pressed = false;
        uint32_t press_duration = current_time - button_press_start_time;
        
        if (!button_processed) {
            if (press_duration >= 50) {  // 最小按下时间
                // 短按：切换屏幕开关
                printf("[按键] 短按检测 (时长: %lu ms) - 切换屏幕\n", press_duration);
                screen_on = !screen_on;
                if (screen_on) {
                    printf("[按键] 屏幕开启\n");
                    // 开启背光
                    gpio_put(ILI9488_PIN_BL, 1);
                } else {
                    printf("[按键] 屏幕关闭\n");
                    // 关闭背光
                    gpio_put(ILI9488_PIN_BL, 0);
                }
                button_processed = true;
            }
        }
    }
    
    return false;
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

void draw_circle(uint16_t x, uint16_t y, uint16_t r, uint32_t color) {
    if (gfx) {
        gfx->drawCircle(x, y, r, color);
    }
}

void draw_filled_circle(uint16_t x, uint16_t y, uint16_t r, uint32_t color) {
    if (gfx) {
        gfx->fillCircle(x, y, r, color);
    }
}

void draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color) {
    if (gfx) {
        gfx->drawLine(x0, y0, x1, y1, color);
    }
}

// 绘制加粗线条
void draw_thick_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color, uint8_t thickness = 3) {
    if (gfx) {
        printf("[绘制调试] draw_thick_line: (%d,%d) -> (%d,%d), 颜色=0x%06X, 厚度=%d\n", 
               x0, y0, x1, y1, color, thickness);
        
        // 计算线条方向向量
        int16_t dx = x1 - x0;
        int16_t dy = y1 - y0;
        float length = sqrt(dx * dx + dy * dy);
        
        if (length == 0) {
            printf("[绘制调试] 线段长度为0，跳过绘制\n");
            return; // 避免除零
        }
        
        printf("[绘制调试] 线段长度: %.2f, dx=%d, dy=%d\n", length, dx, dy);
        
        // 归一化方向向量
        float nx = dx / length;
        float ny = dy / length;
        
        // 计算垂直方向向量
        float perp_x = -ny;
        float perp_y = nx;
        
        // 绘制多条平行线来模拟粗线条
        for (int i = 0; i < thickness; i++) {
            float offset = (i - thickness / 2.0f) * 0.5f;
            int16_t offset_x = (int16_t)(offset * perp_x);
            int16_t offset_y = (int16_t)(offset * perp_y);
            
            printf("[绘制调试] 绘制第%d条线: 偏移(%d,%d), 实际坐标(%d,%d) -> (%d,%d)\n", 
                   i, offset_x, offset_y, 
                   x0 + offset_x, y0 + offset_y, 
                   x1 + offset_x, y1 + offset_y);
            
            gfx->drawLine(x0 + offset_x, y0 + offset_y, x1 + offset_x, y1 + offset_y, color);
        }
        printf("[绘制调试] draw_thick_line 完成\n");
    } else {
        printf("[绘制调试] 错误：gfx对象为空，无法绘制\n");
    }
}

// 绘制加粗圆圈
void draw_thick_circle(uint16_t x, uint16_t y, uint16_t r, uint32_t color, uint8_t thickness = 3) {
    if (gfx) {
        // 绘制多个同心圆来模拟粗圆圈
        for (int i = 0; i < thickness; i++) {
            gfx->drawCircle(x, y, r + i, color);
            if (i > 0) gfx->drawCircle(x, y, r - i, color);
        }
    }
}

// 绘制加粗半圆（用于速度表）
void draw_thick_arc(uint16_t x, uint16_t y, uint16_t r, int16_t start_angle, int16_t end_angle, uint32_t color, uint8_t thickness = 3) {
    // 使用多个同心圆来模拟粗弧线
    for (uint8_t i = 0; i < thickness; i++) {
        // 绘制多个点来模拟弧线
        for (int16_t angle = start_angle; angle <= end_angle; angle += 2) {
            float rad = angle * M_PI / 180.0f;
            int16_t px = x + (r + i) * cos(rad);
            int16_t py = y + (r + i) * sin(rad);
            draw_line(px, py, px, py, color);
        }
    }
}

void draw_string(uint16_t x, uint16_t y, const char* str, uint32_t color, uint32_t bg_color) {
    if (driver && str) {
        driver->drawString(x, y, str, color, bg_color);
    }
}

/**
 * @brief 清除时钟指针（用背景色覆盖）
 */
void clear_clock_hand(uint16_t center_x, uint16_t center_y, int16_t end_x, int16_t end_y, uint8_t thickness, uint32_t bg_color) {
    // 清除指针线条
    draw_thick_line(center_x, center_y, end_x, end_y, bg_color, thickness);
    
    // 清除指针末端的小圆点
    draw_filled_circle(end_x, end_y, thickness + 1, bg_color);
}

/**
 * @brief 计算点到线段的距离
 * 用于精确判断数字是否被指针遮挡
 */
float point_to_line_distance(int16_t px, int16_t py, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    float A = px - x1;
    float B = py - y1;
    float C = x2 - x1;
    float D = y2 - y1;
    
    float dot = A * C + B * D;
    float len_sq = C * C + D * D;
    
    if (len_sq == 0) {
        // 线段退化为点
        return sqrt(A * A + B * B);
    }
    
    float param = dot / len_sq;
    
    float xx, yy;
    if (param < 0) {
        xx = x1;
        yy = y1;
    } else if (param > 1) {
        xx = x2;
        yy = y2;
    } else {
        xx = x1 + param * C;
        yy = y1 + param * D;
    }
    
    float dx = px - xx;
    float dy = py - yy;
    return sqrt(dx * dx + dy * dy);
}

/**
 * @brief 专业电子手表显示算法：智能检测指针遮挡并重绘数字
 * 基于指针路径和数字位置的几何关系，精确计算遮挡区域
 */
void redraw_clock_numbers_affected_by_hand(uint16_t center_x, uint16_t center_y, 
                                          int16_t old_hand_x, int16_t old_hand_y, 
                                          int16_t new_hand_x, int16_t new_hand_y, 
                                          uint8_t hand_thickness) {
    // 计算指针路径的边界框，包含指针厚度
    int16_t min_x = fmin(old_hand_x, new_hand_x) - hand_thickness - 8;
    int16_t max_x = fmax(old_hand_x, new_hand_x) + hand_thickness + 8;
    int16_t min_y = fmin(old_hand_y, new_hand_y) - hand_thickness - 8;
    int16_t max_y = fmax(old_hand_y, new_hand_y) + hand_thickness + 8;
    
    // 检查每个时钟数字是否在指针路径影响范围内
    for (int i = 0; i < 12; i++) {
        float angle = i * 30.0f * M_PI / 180.0f;
        int16_t text_x = center_x + (DIAL_RADIUS - 25) * cos(angle - M_PI/2) - 5;
        int16_t text_y = center_y + (DIAL_RADIUS - 25) * sin(angle - M_PI/2) - 5;
        
        // 检查数字是否在指针路径的边界框内
        bool in_bounds = (text_x >= min_x && text_x <= max_x && 
                         text_y >= min_y && text_y <= max_y);
        
        if (in_bounds) {
            // 进一步检查数字是否真的被指针遮挡
            // 使用点到线段距离算法
            float distance_to_old_line = point_to_line_distance(text_x, text_y, center_x, center_y, old_hand_x, old_hand_y);
            float distance_to_new_line = point_to_line_distance(text_x, text_y, center_x, center_y, new_hand_x, new_hand_y);
            
            // 如果数字距离指针路径小于阈值，则重绘
            float threshold = hand_thickness + 12; // 指针厚度 + 安全边距
            if (distance_to_old_line < threshold || distance_to_new_line < threshold) {
                char hour_str[4];
                snprintf(hour_str, sizeof(hour_str), "%d", (i == 0) ? 12 : i);
                draw_string(text_x, text_y, hour_str, COLOR_WHITE, COLOR_BLACK);
            }
        }
    }
}


void fill_screen(uint32_t color) {
    if (driver) {
        uint16_t color565 = ((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) | ((color >> 3) & 0x001F);
        driver->fillScreen(color565);
    }
}

// =============================================================================
// GPS信号显示函数
// =============================================================================

/**
 * @brief 绘制GPS信号强度指示器（手机信号格样式）
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param strength 信号强度 (0-100)
 */
void draw_gps_signal_strength(uint16_t x, uint16_t y, uint8_t strength) {
    // 5格信号强度显示
    uint8_t bars = 0;
    if (strength >= 80) bars = 5;
    else if (strength >= 60) bars = 4;
    else if (strength >= 40) bars = 3;
    else if (strength >= 20) bars = 2;
    else if (strength > 0) bars = 1;
    
    // 绘制信号格
    for (int i = 0; i < 5; i++) {
        uint16_t bar_width = 4;
        uint16_t bar_height = 6 + i * 3;
        uint16_t bar_x = x + i * 6;
        uint16_t bar_y = y + (20 - bar_height);
        
        uint32_t bar_color = (i < bars) ? COLOR_GREEN : COLOR_DARK_BLUE;
        draw_filled_rect(bar_x, bar_y, bar_width, bar_height, bar_color);
    }
}

/**
 * @brief 绘制GPS卫星数量显示
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param satellites 卫星数量
 */
void draw_gps_satellite_count(uint16_t x, uint16_t y, uint8_t satellites) {
    // 绘制卫星图标
    draw_string(x, y, "SAT", COLOR_WHITE, COLOR_BLACK);
    
    // 绘制卫星数量
    char sat_str[8];
    snprintf(sat_str, sizeof(sat_str), "%d", satellites);
    draw_string(x + 25, y, sat_str, COLOR_CYAN, COLOR_BLACK);
}

// =============================================================================
// GPS数据处理函数
// =============================================================================

void update_gps_data() {
    GNRMC new_data = vendor_gps_get_gnrmc();
    
    // 增加数据包计数
    packet_count++;
    
    // 详细日志打印
    printf("[GPS调试] 数据包 #%lu\n", packet_count);
    printf("[GPS调试] 状态: %d, 纬度: %.6f, 经度: %.6f\n", 
           new_data.Status, new_data.Lat, new_data.Lon);
    printf("[GPS调试] 速度: %.2f, 航向: %.2f\n", 
           new_data.Speed, new_data.Course);
    printf("[GPS调试] 时间: %02d:%02d:%02d, 日期: %s\n",
           new_data.Time_H, new_data.Time_M, new_data.Time_S,
           new_data.Date);
    
    // 检查是否获得有效数据
    bool got_valid_data = false;
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
    
    // 检查GPS状态变化
    bool gps_is_valid = got_valid_data;
    if (gps_is_valid && !gps_was_valid) {
        // GPS从无效变为有效
        gps_valid_start_time = to_ms_since_boot(get_absolute_time());
        printf("[GPS调试] GPS定位成功，开始计时...\n");
        printf("[GPS调试] 有效定位计数: %lu\n", valid_fix_count);
    }
    gps_was_valid = gps_is_valid;
    
    // 更新数据
    if (memcmp(&new_data, &current_gps_data, sizeof(GNRMC)) != 0) {
        // 检查哪些数据发生了变化
        bool course_changed = (fabs(new_data.Course - current_gps_data.Course) > 0.1);
        bool speed_changed = (fabs(new_data.Speed - current_gps_data.Speed) > 0.1);
        bool time_changed = (new_data.Time_H != current_gps_data.Time_H || 
                           new_data.Time_M != current_gps_data.Time_M || 
                           new_data.Time_S != current_gps_data.Time_S);
        bool status_changed = (new_data.Status != current_gps_data.Status);
        
        memcpy(&current_gps_data, &new_data, sizeof(GNRMC));
        gps_data_updated = true;
        last_gps_update = to_ms_since_boot(get_absolute_time());
        
        // 设置局部更新标志
        if (course_changed) need_update_compass = true;
        if (speed_changed) need_update_speedometer = true;
        // 数字时间显示现在使用内部局部刷新，不需要外部标志
        if (status_changed) need_update_status = true;
        
        // 时钟需要每秒更新（秒针移动）
        if (time_changed) need_update_compass = true;
        
        // 模拟GPS信号数据更新
        static uint32_t last_signal_update = 0;
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_signal_update >= 2000) { // 每2秒更新一次信号
            // 模拟GPS信号强度变化
            gps_signal_strength = 20 + (packet_count % 60); // 20-80%
            gps_satellites_count = 3 + (packet_count % 8);  // 3-10颗卫星
            gps_satellites_in_view = 8 + (packet_count % 5); // 8-12颗可见
            
            need_update_gps_signal = true;
            last_signal_update = current_time;
        }
        
        printf("[GPS调试] GPS数据已更新\n");
    }
    
    // 每10个数据包打印一次统计信息
    if (packet_count % 10 == 0) {
        printf("[GPS统计] 总数据包: %lu, 有效定位: %lu, 成功率: %.1f%%\n",
               packet_count, valid_fix_count, 
               packet_count > 0 ? (float)valid_fix_count * 100.0f / packet_count : 0.0f);
    }
}

// =============================================================================
// 仪表盘绘制函数
// =============================================================================

/**
 * @brief 绘制圆形时钟（智能局部刷新版本）
 */
void draw_analog_clock(uint16_t center_x, uint16_t center_y, uint8_t hour, uint8_t minute, uint8_t second) {
    // 首次绘制或需要完全重绘时，绘制时钟背景
    if (!clock_initialized) {
        // 外圈 - 加粗
        draw_thick_circle(center_x, center_y, DIAL_RADIUS, COLOR_MEDIUM_BLUE, 4);
        draw_thick_circle(center_x, center_y, DIAL_RADIUS - 2, COLOR_DARK_BLUE, 2);
        
        // 绘制时钟刻度
        for (int i = 0; i < 12; i++) {
            float angle = i * 30.0f * M_PI / 180.0f; // 每小时30度
            int16_t x1 = center_x + (DIAL_RADIUS - 15) * cos(angle - M_PI/2);
            int16_t y1 = center_y + (DIAL_RADIUS - 15) * sin(angle - M_PI/2);
            int16_t x2 = center_x + (DIAL_RADIUS - 5) * cos(angle - M_PI/2);
            int16_t y2 = center_y + (DIAL_RADIUS - 5) * sin(angle - M_PI/2);
            
            draw_thick_line(x1, y1, x2, y2, COLOR_CYAN, 3);
            
            // 绘制小时数字
            char hour_str[4];
            snprintf(hour_str, sizeof(hour_str), "%d", (i == 0) ? 12 : i);
            int16_t text_x = center_x + (DIAL_RADIUS - 25) * cos(angle - M_PI/2) - 5;
            int16_t text_y = center_y + (DIAL_RADIUS - 25) * sin(angle - M_PI/2) - 5;
            draw_string(text_x, text_y, hour_str, COLOR_WHITE, COLOR_BLACK);
        }
        
        // 绘制分钟刻度（小刻度）
        for (int i = 0; i < 60; i++) {
            if (i % 5 != 0) { // 跳过小时刻度
                float angle = i * 6.0f * M_PI / 180.0f; // 每分钟6度
                int16_t x1 = center_x + (DIAL_RADIUS - 10) * cos(angle - M_PI/2);
                int16_t y1 = center_y + (DIAL_RADIUS - 10) * sin(angle - M_PI/2);
                int16_t x2 = center_x + (DIAL_RADIUS - 5) * cos(angle - M_PI/2);
                int16_t y2 = center_y + (DIAL_RADIUS - 5) * sin(angle - M_PI/2);
                
                draw_line(x1, y1, x2, y2, COLOR_CYAN);
            }
        }
        
        // 中心点
        draw_filled_circle(center_x, center_y, 6, COLOR_WHITE);
        draw_filled_circle(center_x, center_y, 3, COLOR_RED);
        
        clock_initialized = true;
    }
    
    // 计算当前指针角度（移除秒针）
    float hour_angle = ((hour % 12) * 30.0f + minute * 0.5f) * M_PI / 180.0f;
    float minute_angle = (minute * 6.0f) * M_PI / 180.0f;
    
    // 计算当前指针位置
    int16_t hour_x = center_x + (DIAL_RADIUS - 40) * cos(hour_angle - M_PI/2);
    int16_t hour_y = center_y + (DIAL_RADIUS - 40) * sin(hour_angle - M_PI/2);
    int16_t minute_x = center_x + (DIAL_RADIUS - 25) * cos(minute_angle - M_PI/2);
    int16_t minute_y = center_y + (DIAL_RADIUS - 25) * sin(minute_angle - M_PI/2);
    
    // 计算上一帧指针位置
    float last_hour_angle = ((last_clock_hour % 12) * 30.0f + last_clock_minute * 0.5f) * M_PI / 180.0f;
    float last_minute_angle = (last_clock_minute * 6.0f) * M_PI / 180.0f;
    
    int16_t last_hour_x = center_x + (DIAL_RADIUS - 40) * cos(last_hour_angle - M_PI/2);
    int16_t last_hour_y = center_y + (DIAL_RADIUS - 40) * sin(last_hour_angle - M_PI/2);
    int16_t last_minute_x = center_x + (DIAL_RADIUS - 25) * cos(last_minute_angle - M_PI/2);
    int16_t last_minute_y = center_y + (DIAL_RADIUS - 25) * sin(last_minute_angle - M_PI/2);
    
    // 专业电子手表显示算法：智能指针遮挡检测和局部刷新
    
    // 1. 分针更新：每分钟变化
    if (minute != last_clock_minute) {
        // 清除旧分针
        clear_clock_hand(center_x, center_y, last_minute_x, last_minute_y, 4, COLOR_BLACK);
        // 重绘被分针遮挡的数字
        redraw_clock_numbers_affected_by_hand(center_x, center_y, last_minute_x, last_minute_y, minute_x, minute_y, 4);
        // 绘制新分针
        draw_thick_line(center_x, center_y, minute_x, minute_y, COLOR_CYAN, 4);
    }
    
    // 2. 时针更新：每小时变化，或分针移动时微调
    if (hour != last_clock_hour || minute != last_clock_minute) {
        // 清除旧时针
        clear_clock_hand(center_x, center_y, last_hour_x, last_hour_y, 4, COLOR_BLACK);
        // 重绘被时针遮挡的数字
        redraw_clock_numbers_affected_by_hand(center_x, center_y, last_hour_x, last_hour_y, hour_x, hour_y, 4);
        // 绘制新时针
        draw_thick_line(center_x, center_y, hour_x, hour_y, COLOR_WHITE, 4);
    }
    
    // 数字时间显示已移除，界面更简洁
    
    // 保存当前时间用于下次比较
    last_clock_hour = hour;
    last_clock_minute = minute;
    last_clock_second = second;
}

/**
 * @brief 绘制罗盘仪表盘（保留原函数，但不再使用）
 */
void draw_compass_dial(uint16_t center_x, uint16_t center_y, float heading) {
    // 外圈 - 加粗
    draw_thick_circle(center_x, center_y, DIAL_RADIUS, COLOR_MEDIUM_BLUE, 4);
    draw_thick_circle(center_x, center_y, DIAL_RADIUS - 2, COLOR_DARK_BLUE, 2);
    
    // 绘制方向刻度
    const char* directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    for (int i = 0; i < 8; i++) {
        float angle = i * 45.0f * M_PI / 180.0f;
        int16_t x1 = center_x + (DIAL_RADIUS - 15) * cos(angle);
        int16_t y1 = center_y + (DIAL_RADIUS - 15) * sin(angle);
        int16_t x2 = center_x + (DIAL_RADIUS - 5) * cos(angle);
        int16_t y2 = center_y + (DIAL_RADIUS - 5) * sin(angle);
        
        draw_thick_line(x1, y1, x2, y2, COLOR_CYAN, 3);
        
        // 绘制方向文字
        int16_t text_x = center_x + (DIAL_RADIUS - 25) * cos(angle) - 5;
        int16_t text_y = center_y + (DIAL_RADIUS - 25) * sin(angle) - 5;
        draw_string(text_x, text_y, directions[i], COLOR_WHITE, COLOR_BLACK);
    }
    
    // 绘制数字刻度
    for (int i = 0; i < 8; i++) {
        float angle = i * 45.0f * M_PI / 180.0f;
        int16_t x1 = center_x + (DIAL_RADIUS - 30) * cos(angle);
        int16_t y1 = center_y + (DIAL_RADIUS - 30) * sin(angle);
        int16_t x2 = center_x + (DIAL_RADIUS - 35) * cos(angle);
        int16_t y2 = center_y + (DIAL_RADIUS - 35) * sin(angle);
        
        draw_thick_line(x1, y1, x2, y2, COLOR_CYAN, 2);
    }
    
    // 绘制方向指针
    float heading_rad = heading * M_PI / 180.0f;
    int16_t pointer_x = center_x + (DIAL_RADIUS - 20) * cos(heading_rad - M_PI/2);
    int16_t pointer_y = center_y + (DIAL_RADIUS - 20) * sin(heading_rad - M_PI/2);
    
    // 指针主体 - 加粗
    draw_thick_line(center_x, center_y, pointer_x, pointer_y, COLOR_BRIGHT_BLUE, 4);
    
    // 指针尖端
    draw_filled_circle(pointer_x, pointer_y, 3, COLOR_BRIGHT_BLUE);
    
    // 中心点
    draw_filled_circle(center_x, center_y, 4, COLOR_WHITE);
    
    // 显示当前角度
    char angle_str[16];
    snprintf(angle_str, sizeof(angle_str), "%.0f°", heading);
    draw_string(center_x - 15, center_y + DIAL_RADIUS + 10, angle_str, COLOR_WHITE, COLOR_BLACK);
}

/**
 * @brief 绘制速度仪表盘静态部分（刻度、外圈等）
 */
void draw_speedometer_static(uint16_t center_x, uint16_t center_y) {
    // 外圈 - 加粗
    draw_thick_circle(center_x, center_y, DIAL_RADIUS, COLOR_MEDIUM_BLUE, 4);
    draw_thick_circle(center_x, center_y, DIAL_RADIUS - 2, COLOR_DARK_BLUE, 2);
    
    // 绘制速度刻度 (0-120 km/h)
    // 大刻度线：每20km一条，带数字
    for (int i = 0; i <= 6; i++) {
        int speed_value = i * 20;
        float angle = (speed_value / 120.0f * 180.0f - 90.0f) * M_PI / 180.0f;
        
        int16_t x1 = center_x + (DIAL_RADIUS - 15) * cos(angle);
        int16_t y1 = center_y + (DIAL_RADIUS - 15) * sin(angle);
        int16_t x2 = center_x + (DIAL_RADIUS - 5) * cos(angle);
        int16_t y2 = center_y + (DIAL_RADIUS - 5) * sin(angle);
        
        // 超过100km/h的刻线用鲜红色
        uint32_t mark_color = (speed_value > 100) ? COLOR_RED : COLOR_CYAN;
        draw_thick_line(x1, y1, x2, y2, mark_color, 3);
        
        // 绘制速度数字
        char speed_str[8];
        snprintf(speed_str, sizeof(speed_str), "%d", speed_value);
        int16_t text_x = center_x + (DIAL_RADIUS - 25) * cos(angle) - 8;
        int16_t text_y = center_y + (DIAL_RADIUS - 25) * sin(angle) - 5;
        draw_string(text_x, text_y, speed_str, COLOR_WHITE, COLOR_BLACK);
    }
    
    // 小刻度线：每5km一条
    for (int i = 1; i <= 23; i++) {
        int speed_value = i * 5;
        if (speed_value % 20 == 0) continue; // 跳过已经绘制的大刻度线
        
        float angle = (speed_value / 120.0f * 180.0f - 90.0f) * M_PI / 180.0f;
        
        int16_t x1 = center_x + (DIAL_RADIUS - 12) * cos(angle);
        int16_t y1 = center_y + (DIAL_RADIUS - 12) * sin(angle);
        int16_t x2 = center_x + (DIAL_RADIUS - 8) * cos(angle);
        int16_t y2 = center_y + (DIAL_RADIUS - 8) * sin(angle);
        
        // 超过100km/h的小刻线也用鲜红色
        uint32_t mark_color = (speed_value > 100) ? COLOR_RED : COLOR_CYAN;
        draw_line(x1, y1, x2, y2, mark_color);
    }
    
    // 中心点（静态部分）
    draw_filled_circle(center_x, center_y, 6, COLOR_WHITE);
    draw_filled_circle(center_x, center_y, 3, COLOR_RED);
}

/**
 * @brief 重绘时速计中被指针遮挡的数字
 */
void redraw_speedometer_numbers_affected_by_pointer(uint16_t center_x, uint16_t center_y, 
                                                   int16_t old_pointer_x, int16_t old_pointer_y, 
                                                   int16_t new_pointer_x, int16_t new_pointer_y, 
                                                   uint8_t pointer_thickness) {
    // 计算指针路径的边界框，包含指针厚度
    int16_t min_x = fmin(old_pointer_x, new_pointer_x) - pointer_thickness - 10;
    int16_t max_x = fmax(old_pointer_x, new_pointer_x) + pointer_thickness + 10;
    int16_t min_y = fmin(old_pointer_y, new_pointer_y) - pointer_thickness - 10;
    int16_t max_y = fmax(old_pointer_y, new_pointer_y) + pointer_thickness + 10;
    
    // 检查每个时速计数字是否在指针路径影响范围内
    for (int i = 0; i <= 6; i++) {
        int speed_value = i * 20;
        float angle = (speed_value / 120.0f * 180.0f - 90.0f) * M_PI / 180.0f;
        
        int16_t text_x = center_x + (DIAL_RADIUS - 25) * cos(angle) - 8;
        int16_t text_y = center_y + (DIAL_RADIUS - 25) * sin(angle) - 5;
        
        // 检查数字是否在指针路径的边界框内
        bool in_bounds = (text_x >= min_x && text_x <= max_x && 
                         text_y >= min_y && text_y <= max_y);
        
        if (in_bounds) {
            // 重绘这个数字
            char speed_str[8];
            snprintf(speed_str, sizeof(speed_str), "%d", speed_value);
            draw_string(text_x, text_y, speed_str, COLOR_WHITE, COLOR_BLACK);
        }
    }
}

/**
 * @brief 绘制速度仪表盘指针部分（动态更新）
 */
void draw_speedometer_pointer(uint16_t center_x, uint16_t center_y, float speed) {
    static float last_speed = -1.0f;
    static int16_t last_pointer_x = 0, last_pointer_y = 0;
    
    printf("[时速计调试] 开始绘制指针: center_x=%d, center_y=%d, speed=%.2f\n", center_x, center_y, speed);
    
    // 绘制速度指针
    // 如果速度小于1km/h，指针指向0
    float display_speed = (speed < 1.0f) ? 0.0f : speed;
    // 修复角度计算：0速度对应-90度（顶部），120速度对应90度（底部）
    float speed_angle = (display_speed / 120.0f * 180.0f - 90.0f) * M_PI / 180.0f;
    speed_angle = fmaxf(-M_PI/2, fminf(M_PI/2, speed_angle)); // 限制在-90°到90°
    
    // 调整指针长度，确保指向正确的刻度位置
    int16_t pointer_length = DIAL_RADIUS - 15; // 从20改为15，让指针更接近刻度
    int16_t pointer_x = center_x + pointer_length * cos(speed_angle);
    int16_t pointer_y = center_y + pointer_length * sin(speed_angle);
    
    printf("[时速计调试] 计算参数: display_speed=%.2f, speed_angle=%.2f度, pointer_x=%d, pointer_y=%d\n", 
           display_speed, speed_angle * 180.0f / M_PI, pointer_x, pointer_y);
    printf("[时速计调试] 仪表参数: DIAL_RADIUS=%d, 指针长度=%d\n", DIAL_RADIUS, pointer_length);
    
    // 智能局部刷新：只在指针位置变化时更新
    if (fabs(speed - last_speed) > 0.1f || last_speed < 0) {
        printf("[时速计调试] 指针需要更新: last_speed=%.2f, 差值=%.2f\n", last_speed, fabs(speed - last_speed));
        
        // 清除旧指针（如果存在）
        if (last_speed >= 0) {
            printf("[时速计调试] 清除旧指针: (%d,%d) -> (%d,%d)\n", center_x, center_y, last_pointer_x, last_pointer_y);
            draw_thick_line(center_x, center_y, last_pointer_x, last_pointer_y, COLOR_BLACK, 12);
            draw_filled_circle(last_pointer_x, last_pointer_y, 8, COLOR_BLACK);
            draw_filled_circle(center_x, center_y, 8, COLOR_BLACK);
            
            // 重绘被旧指针遮挡的数字
            redraw_speedometer_numbers_affected_by_pointer(center_x, center_y, 
                                                          last_pointer_x, last_pointer_y, 
                                                          last_pointer_x, last_pointer_y, 12);
        }
        
        // 绘制新指针 - 使用白色指针确保可见
        printf("[时速计调试] 绘制新白色指针: (%d,%d) -> (%d,%d), 颜色=0x%06X\n", 
               center_x, center_y, pointer_x, pointer_y, COLOR_WHITE);
        
        // 先绘制一个更粗的指针确保可见
        draw_thick_line(center_x, center_y, pointer_x, pointer_y, COLOR_WHITE, 12);
        draw_filled_circle(pointer_x, pointer_y, 8, COLOR_WHITE);
        
        // 再绘制中心点确保指针根部可见
        draw_filled_circle(center_x, center_y, 8, COLOR_WHITE);
        
        // 重绘被新指针遮挡的数字
        redraw_speedometer_numbers_affected_by_pointer(center_x, center_y, 
                                                      last_pointer_x, last_pointer_y, 
                                                      pointer_x, pointer_y, 12);
        
        last_speed = speed;
        last_pointer_x = pointer_x;
        last_pointer_y = pointer_y;
        
        printf("[时速计调试] 指针绘制完成，保存新位置: (%d,%d)\n", pointer_x, pointer_y);
    } else {
        printf("[时速计调试] 指针位置未变化，跳过绘制\n");
    }
    
    // 显示当前速度（在底部中央）
    char speed_str[16];
    snprintf(speed_str, sizeof(speed_str), "%.1f km/h", speed);
    draw_string(SPEED_X - 30, SPEED_Y, speed_str, COLOR_GREEN, COLOR_BLACK);
    printf("[时速计调试] 速度文字显示: '%s' 位置(%d,%d)\n", speed_str, SPEED_X - 30, SPEED_Y);
}

// draw_time_display 函数已移除，界面更简洁

/**
 * @brief 绘制状态栏
 */
void draw_status_bar() {
    // 完全移除状态栏绘制，避免遮挡数字时钟
    // 状态栏背景已移除
}


/**
 * @brief 绘制仪表盘界面
 */
void draw_dashboard_screen() {
    static bool dashboard_initialized = false;
    
    // 首次绘制时绘制所有静态元素
    if (!dashboard_initialized) {
        printf("[仪表盘调试] 首次初始化仪表盘\n");
        printf("[仪表盘调试] 右仪表盘位置: center_x=%d, center_y=%d\n", RIGHT_DIAL_CENTER_X, DIAL_CENTER_Y);
        
        // 绘制时速计静态部分（刻度、外圈等）
        printf("[仪表盘调试] 绘制时速计静态部分\n");
        draw_speedometer_static(RIGHT_DIAL_CENTER_X, DIAL_CENTER_Y);
        
        // 强制绘制指针，确保速度为0时也显示白色指针指向0
        printf("[仪表盘调试] 强制绘制初始指针（速度为0）\n");
        draw_speedometer_pointer(RIGHT_DIAL_CENTER_X, DIAL_CENTER_Y, 0.0f);
        
        dashboard_initialized = true;
        printf("[仪表盘调试] 仪表盘初始化完成\n");
    }
    
    // 只在需要时进行局部更新
    if (need_update_compass) {
        // 使用GPS时间绘制时钟
        draw_analog_clock(LEFT_DIAL_CENTER_X, DIAL_CENTER_Y, 
                         current_gps_data.Time_H, current_gps_data.Time_M, current_gps_data.Time_S);
        need_update_compass = false;
    }
    
    if (need_update_speedometer) {
        float speed = current_gps_data.Speed;
        printf("[仪表盘调试] 时速计需要更新，当前速度: %.2f km/h\n", speed);
        draw_speedometer_pointer(RIGHT_DIAL_CENTER_X, DIAL_CENTER_Y, speed);
        need_update_speedometer = false;
    }
    
    // 数字时间显示已移除，界面更简洁
    
    if (need_update_status) {
        draw_status_bar();
        need_update_status = false;
    }
    
    if (need_update_gps_signal) {
        // 绘制GPS信号信息（顶部区域）- 使用LC76G增强功能
        uint8_t satellite_count = vendor_gps_get_satellite_count();
        uint8_t signal_strength = vendor_gps_get_signal_strength();
        draw_gps_satellite_count(10, 5, satellite_count);
        draw_gps_signal_strength(SCREEN_WIDTH - 40, 5, signal_strength);
        need_update_gps_signal = false;
    }
}

// =============================================================================
// 主程序
// =============================================================================

/**
 * @brief GPS仪表盘显示示例主函数
 */
extern "C" int vendor_gps_dashboard_demo() {
    printf("\n=== LC76X GPS + ILI9488仪表盘示例 ===\n");
    printf("分辨率: 480x320\n");
    printf("功能: GPS信息显示 + 高科技仪表盘\n");
    printf("编译时间: %s %s\n", __DATE__, __TIME__);
    
    // 设置调试模式
    vendor_gps_set_debug(enable_debug);
    printf("调试模式: %s\n", enable_debug ? "已启用" : "已禁用");
    
    // 启用详细日志
    printf("启用详细GPS调试日志\n");
    
    // 记录系统启动时间
    system_start_time = to_ms_since_boot(get_absolute_time());
    
    // 初始化GPS模块
    printf("正在初始化GPS模块...\n");
    printf("[GPS调试] UART ID: %d, 波特率: %d\n", GPS_UART_ID, GPS_BAUD_RATE);
    printf("[GPS调试] TX引脚: %d, RX引脚: %d, FORCE引脚: %d\n", 
           GPS_TX_PIN, GPS_RX_PIN, GPS_FORCE_PIN);
    
    if (!vendor_gps_init(GPS_UART_ID, GPS_BAUD_RATE, GPS_TX_PIN, GPS_RX_PIN, GPS_FORCE_PIN)) {
        printf("错误：GPS模块初始化失败\n");
        return -1;
    }
    
    printf("GPS初始化成功\n");
    
    // LC76G增强配置
    printf("配置LC76G模块...\n");
    
    // 设置定位速率为500ms（2Hz）
    if (vendor_gps_set_positioning_rate(500)) {
        printf("定位速率设置成功: 500ms\n");
    } else {
        printf("定位速率设置失败\n");
    }
    
    // 启用多卫星系统（GPS + GLONASS + Galileo + BDS）
    if (vendor_gps_set_satellite_systems(1, 1, 1, 1, 0)) {
        printf("卫星系统配置成功: GPS+GLONASS+Galileo+BDS\n");
    } else {
        printf("卫星系统配置失败\n");
    }
    
    // 设置NMEA消息输出速率
    vendor_gps_set_nmea_output_rate(0, 1); // GGA每1次定位输出
    vendor_gps_set_nmea_output_rate(3, 1); // GSV每1次定位输出
    vendor_gps_set_nmea_output_rate(4, 1); // RMC每1次定位输出
    
    // 保存配置到Flash
    if (vendor_gps_save_config()) {
        printf("LC76G配置已保存到Flash\n");
    } else {
        printf("LC76G配置保存失败\n");
    }
    printf("[GPS调试] GPS模块已就绪，等待数据...\n");
    
    // 发送设置命令
    printf("[GPS调试] 发送GPS配置命令...\n");
    vendor_gps_send_command("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    printf("[GPS调试] 已发送PMTK314命令\n");
    sleep_ms(100);
    vendor_gps_send_command("$PMTK220,1000");
    printf("[GPS调试] 已发送PMTK220命令\n");
    sleep_ms(500);
    printf("[GPS调试] GPS配置完成，开始接收数据\n");
    
    // 初始化按键
    printf("正在初始化按键控制...\n");
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);  // 启用内部上拉电阻
    printf("按键初始化完成 (GPIO-%d)\n", BUTTON_PIN);
    
    // 设置随机数种子
    srand(time_us_32());
    
    // 初始化显示驱动
    driver = new ILI9488Driver(ILI9488_GET_SPI_CONFIG());
    gfx = new PicoILI9488GFX<ILI9488Driver>(*driver, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    if (!driver->initialize()) {
        printf("错误：ILI9488驱动初始化失败\n");
        return -1;
    }
    
    // 设置屏幕参数 - 改为横屏模式
    driver->setRotation(Rotation::Landscape_90);  // 横屏模式 (320x480)
    driver->setBacklight(true);
    
    printf("显示驱动初始化成功\n");
    
    // 初始化GPS数据结构
    memset(&current_gps_data, 0, sizeof(GNRMC));
    
    // 显示启动画面
    fill_screen(COLOR_BLACK);
    draw_string(SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 20, "GPS Dashboard", COLOR_WHITE, COLOR_BLACK);
    draw_string(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2, "Initializing...", COLOR_YELLOW, COLOR_BLACK);
    sleep_ms(1000);
    
    // 立即尝试获取GPS数据
    update_gps_data();
    
    // 绘制初始界面
    fill_screen(COLOR_BLACK);
    draw_dashboard_screen();
    
    // 初始化局部更新标志 - 首次绘制时全部更新
    need_update_compass = true;
    need_update_speedometer = true;
    // 数字时间显示现在使用内部局部刷新，不需要外部标志
    need_update_status = true;
    
    printf("系统初始化完成，开始运行...\n");
    
    // 主循环
    uint32_t last_gps_update = 0;
    uint32_t last_display_update = 0;
    
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // 检测按键事件（只处理屏幕开关）
        check_button_events();
        
        // 每秒更新GPS数据
        if (current_time - last_gps_update >= 1000) {
            printf("[主循环] 更新GPS数据 (运行时间: %lu秒)\n", 
                   (current_time - system_start_time) / 1000);
            update_gps_data();
            last_gps_update = current_time;
        }
        
        
        // 更新显示（仅在屏幕开启时）
        if (screen_on && current_time - last_display_update >= 100) { // 10Hz更新
            draw_dashboard_screen();
            last_display_update = current_time;
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
    return vendor_gps_dashboard_demo();
}
