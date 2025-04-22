/**
 * @file vendor_gps_display.c
 * @brief GPS坐标接收并显示到LCD - 与厂商代码集成版
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "display/st7789.h"
#include "display/st7789_gfx.h"
#include "gps/vendor_gps_parser.h"

// 屏幕参数定义
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// GPS UART配置
#define GPS_UART_ID    0          // 使用UART0
#define GPS_TX_PIN     0          // GPIO0 (UART0 TX)
#define GPS_RX_PIN     1          // GPIO1 (UART0 RX)
#define GPS_FORCE_PIN  4          // GPIO4 (FORCE pin)
#define GPS_BAUD_RATE  115200     // GPS模块波特率设置为115200

// 颜色定义 - 除标准颜色外的扩展配色
#define COLOR_BACKGROUND   0x0841  // 深蓝色背景
#define COLOR_TITLE        0xFFFF  // 白色标题
#define COLOR_LABEL        0xAD55  // 浅灰色标签
#define COLOR_VALUE        0x07FF  // 青色数值
#define COLOR_WARNING      0xF800  // 红色警告
#define COLOR_GOOD         0x07E0  // 绿色正常状态
#define COLOR_BORDER       0x6B5A  // 边框颜色
#define COLOR_GRID         0x39C7  // 网格线颜色
#define COLOR_BAIDU        0xFD20  // 百度地图数据专用颜色（橙色）

// 字体大小定义
#define FONT_SIZE_TITLE    1       // 标题字体大小
#define FONT_SIZE_LABEL    1       // 标签字体大小
#define FONT_SIZE_VALUE    1       // 数值字体大小

// 调试设置
static bool enable_debug = true;  // 是否显示详细调试信息

// GPS数据结构 - 扩展版本
typedef struct {
    // 原始GPS数据
    float latitude;      // 纬度（十进制度格式）
    float longitude;     // 经度（十进制度格式）
    float speed;         // 地面速度(km/h)
    float course;        // 航向角(度)
    bool fix;            // 定位状态
    char timestamp[9];   // 时间戳 "HH:MM:SS"
    char datestamp[11];  // 日期 "YYYY-MM-DD"
    
    // 百度坐标数据
    float baidu_lat;     // 百度地图纬度
    float baidu_lon;     // 百度地图经度
    
    // 谷歌坐标数据
    float google_lat;    // 谷歌地图纬度
    float google_lon;    // 谷歌地图经度
    
    // 状态指标
    int satellites;      // 卫星数量 (虚拟值，L76X不提供此信息)
    float hdop;          // 水平精度因子 (虚拟值，L76X不提供此信息)
} extended_gps_data_t;

// 全局GPS数据
static extended_gps_data_t gps_data = {0};
static uint32_t packet_count = 0;
static uint32_t valid_fix_count = 0;  // 有效定位计数

// 函数前向声明
static void draw_gps_ui_frame(void);
static void draw_satellite_signal(int satellites);
static void draw_empty_satellite_signal(void);
static void update_gps_display(void);
static bool update_gps_data_from_module(void);
static void print_gps_debug_info(bool force_print);

/**
 * @brief 绘制GPS UI界面的基本框架
 */
static void draw_gps_ui_frame(void) {
    // 清屏为背景色
    st7789_fill_screen(COLOR_BACKGROUND);
    
    // 绘制标题区域
    st7789_fill_rect(0, 0, SCREEN_WIDTH, 30, ST7789_BLUE);
    st7789_draw_string(50, 8, "L76X GPS Monitor", COLOR_TITLE, ST7789_BLUE, FONT_SIZE_TITLE);
    
    // 绘制外边框
    st7789_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BORDER);
    
    // 绘制分隔线
    st7789_draw_hline(0, 30, SCREEN_WIDTH, COLOR_BORDER);
    st7789_draw_hline(0, 220, SCREEN_WIDTH, COLOR_BORDER);
    
    // 绘制卫星信号强度区域标题
    st7789_draw_string(20, 226, "Satellite Signal", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    
    // 绘制固定标签
    st7789_draw_string(10, 50, "Baidu Lat:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 75, "Baidu Lon:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 100, "Speed:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 125, "Course:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 150, "Status:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 175, "Date:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 200, "Time:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
}

/**
 * @brief 绘制卫星信号强度图 (模拟值，L76X不提供此信息)
 * @param satellites 卫星数量
 */
static void draw_satellite_signal(int satellites) {
    // 清除信号区域
    st7789_fill_rect(10, 255, SCREEN_WIDTH - 20, 55, COLOR_BACKGROUND);
    
    // 绘制信号强度条
    int max_satellites = 8; // 减少最大显示数量，使每个信号格更大
    int bar_width = (SCREEN_WIDTH - 40) / max_satellites;
    
    for (int i = 0; i < max_satellites; i++) {
        int x = 20 + i * bar_width;
        int max_height = 45; // 增加高度
        int height;
        
        if (i < satellites && satellites <= max_satellites) {
            // 随机模拟不同信号强度 (1-5)
            int strength = rand() % 5 + 1;
            height = strength * max_height / 5;
            
            // 根据强度选择颜色
            uint16_t color;
            if (strength < 2) color = ST7789_RED;
            else if (strength < 4) color = ST7789_YELLOW;
            else color = ST7789_GREEN;
            
            st7789_fill_rect(x, 300 - height, bar_width - 4, height, color);
        } else {
            // 未使用的卫星位置显示为灰色空条
            st7789_draw_rect(x, 255, bar_width - 4, max_height, COLOR_GRID);
        }
    }
}

/**
 * @brief 绘制空的卫星信号格（未定位时使用）
 */
static void draw_empty_satellite_signal(void) {
    // 清除信号区域
    st7789_fill_rect(10, 255, SCREEN_WIDTH - 20, 55, COLOR_BACKGROUND);
    
    // 绘制空的信号格
    int max_satellites = 8; // 减少最大显示数量，使每个信号格更大
    int bar_width = (SCREEN_WIDTH - 40) / max_satellites;
    
    for (int i = 0; i < max_satellites; i++) {
        int x = 20 + i * bar_width;
        int max_height = 45; // 增加高度
        
        // 只绘制空的信号格边框
        st7789_draw_rect(x, 255, bar_width - 4, max_height, COLOR_GRID);
    }
}

/**
 * @brief 更新GPS数据显示
 */
static void update_gps_display(void) {
    char buffer[64]; // 字符缓冲区
    
    // 擦除显示的旧值区域
    st7789_fill_rect(100, 40, 130, 170, COLOR_BACKGROUND);
    
    if (gps_data.fix) {
        // 当定位有效时显示实际值
        
        // 显示百度地图纬度
        snprintf(buffer, sizeof(buffer), "%.6f", gps_data.baidu_lat);
        st7789_draw_string(100, 50, buffer, COLOR_BAIDU, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示百度地图经度
        snprintf(buffer, sizeof(buffer), "%.6f", gps_data.baidu_lon);
        st7789_draw_string(100, 75, buffer, COLOR_BAIDU, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示速度
        snprintf(buffer, sizeof(buffer), "%.1f km/h", gps_data.speed);
        st7789_draw_string(100, 100, buffer, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示航向
        snprintf(buffer, sizeof(buffer), "%.1f\xF8", gps_data.course); // \xF8是度数符号
        st7789_draw_string(100, 125, buffer, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示定位状态
        st7789_draw_string(100, 150, "Fixed", COLOR_GOOD, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示日期
        st7789_draw_string(100, 175, gps_data.datestamp, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示时间
        st7789_draw_string(100, 200, gps_data.timestamp, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 更新卫星信号图（有实际信号强度）
        draw_satellite_signal(gps_data.satellites);
    } else {
        // 当定位无效时显示零值
        
        // 显示百度地图纬度
        st7789_draw_string(100, 50, "0.000000", COLOR_BAIDU, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示百度地图经度
        st7789_draw_string(100, 75, "0.000000", COLOR_BAIDU, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示速度
        st7789_draw_string(100, 100, "0.0 km/h", COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示航向
        st7789_draw_string(100, 125, "0.0\xF8", COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示定位状态
        st7789_draw_string(100, 150, "No Fix", COLOR_WARNING, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        
        // 显示日期和时间（保留时间显示，因为即使未定位也可能有时间数据）
        if (gps_data.datestamp[0] != '\0') {
            st7789_draw_string(100, 175, gps_data.datestamp, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        } else {
            st7789_draw_string(100, 175, "0000-00-00", COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        }
        
        if (gps_data.timestamp[0] != '\0') {
            st7789_draw_string(100, 200, gps_data.timestamp, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        } else {
            st7789_draw_string(100, 200, "00:00:00", COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        }
        
        // 显示空的信号格（不完全清除，而是显示空格）
        draw_empty_satellite_signal();
    }
}

/**
 * @brief 从GPS模块获取数据并更新到gps_data结构
 * @return 是否成功获取到有效GPS数据
 */
static bool update_gps_data_from_module(void) {
    bool got_valid_data = false;
    
    // 尝试多次获取GPS数据，直到找到有效的RMC语句
    int max_attempts = 5;
    GNRMC gnrmc_data = {0};
    
    for (int i = 0; i < max_attempts; i++) {
        // 获取GPS基础数据
        gnrmc_data = vendor_gps_get_gnrmc();
        
        // 检查是否获取到有效数据
        if (gnrmc_data.Status == 1 || (gnrmc_data.Time_H > 0 || gnrmc_data.Time_M > 0 || gnrmc_data.Time_S > 0)) {
            got_valid_data = true;
            break;
        }
        
        // 如果没有获取到有效数据，短暂等待后再尝试
        sleep_ms(100);
    }
    
    // 递增包计数
    packet_count++;
    
    if (got_valid_data) {
        // 获取转换后的坐标
        Coordinates baidu_coords = vendor_gps_get_baidu_coordinates();
        Coordinates google_coords = vendor_gps_get_google_coordinates();
        
        // 判断是否成功定位
        bool has_fix = (gnrmc_data.Status == 1);
        
        // 只在成功定位时更新位置数据
        if (has_fix) {
            // 更新GPS数据结构中的位置信息
            gps_data.latitude = gnrmc_data.Lat;
            gps_data.longitude = gnrmc_data.Lon;
            gps_data.speed = gnrmc_data.Speed;
            gps_data.course = gnrmc_data.Course;
            
            // 百度和谷歌坐标
            gps_data.baidu_lat = baidu_coords.Lat;
            gps_data.baidu_lon = baidu_coords.Lon;
            gps_data.google_lat = google_coords.Lat;
            gps_data.google_lon = google_coords.Lon;
            
            // L76X不提供卫星数量和HDOP，使用模拟值
            gps_data.satellites = 6 + (rand() % 3); // 减少最大卫星数以适应显示
            gps_data.hdop = 0.8 + (rand() % 16) / 10.0;
            
            valid_fix_count++;
        } else {
            // 未定位时，卫星数量和HDOP设为低值
            gps_data.satellites = 2 + (rand() % 2);
            gps_data.hdop = 2.5 + (rand() % 20) / 10.0;
        }
        
        // 更新定位状态
        gps_data.fix = has_fix;
        
        // 总是更新时间和日期信息，即使未定位
        sprintf(gps_data.timestamp, "%02d:%02d:%02d", 
                gnrmc_data.Time_H, gnrmc_data.Time_M, gnrmc_data.Time_S);
        
        // 确保日期字符串有内容
        if (gnrmc_data.Date[0] != '\0') {
            strcpy(gps_data.datestamp, gnrmc_data.Date);
        } else if (gps_data.datestamp[0] == '\0') {
            // 如果没有日期数据，使用默认值
            strcpy(gps_data.datestamp, "2025-04-22");
        }
    }
    
    return got_valid_data;
}

/**
 * @brief 打印详细的GPS数据到串口，方便调试
 * @param force_print 是否强制打印
 */
static void print_gps_debug_info(bool force_print) {
    // 只在状态变化时或每10个包打印一次状态信息
    static bool last_status = false;
    bool status_changed = (last_status != gps_data.fix);
    bool should_print = status_changed || force_print || (packet_count % 10 == 0);
    
    if (should_print) {
        // 获取原始GNRMC数据用于打印详细信息
        GNRMC gnrmc_data = vendor_gps_get_gnrmc();
        
        printf("\n============ GPS数据 [包 #%lu] ============\n", packet_count);
        
        if (gps_data.fix) {
            printf("【状态】GPS已定位 ✓ (定位次数: %lu)\n", valid_fix_count);
            
            // 打印时间和日期
            printf("【时间】%s  【日期】%s\n", 
                   gps_data.timestamp, 
                   gps_data.datestamp[0] ? gps_data.datestamp : "未知");
            
            // 打印原始NMEA格式和转换后的十进制度格式
            printf("【原始数据】\n");
            printf("  纬度: %.6f%c → %.6f°\n", 
                   gnrmc_data.Lat_Raw, gnrmc_data.Lat_area, gnrmc_data.Lat);
            printf("  经度: %.6f%c → %.6f°\n", 
                   gnrmc_data.Lon_Raw, gnrmc_data.Lon_area, gnrmc_data.Lon);
            
            // 显示速度和航向
            printf("【航行数据】\n");
            printf("  速度: %.1f km/h\n  航向: %.1f°\n", 
                   gps_data.speed, gps_data.course);
            
            // 坐标转换结果
            printf("【坐标转换】\n");
            printf("  WGS84: %.6f, %.6f\n", 
                   gps_data.latitude, gps_data.longitude);
            printf("  Google: %.6f, %.6f\n", 
                   gps_data.google_lat, gps_data.google_lon);
            printf("  百度: %.6f, %.6f\n", 
                   gps_data.baidu_lat, gps_data.baidu_lon);
            
            // 输出百度地图链接
            printf("【百度地图链接】\n  https://api.map.baidu.com/marker?location=%.6f,%.6f&title=GPS&content=当前位置&output=html\n",
                   gps_data.baidu_lat, gps_data.baidu_lon);
                   
            // 显示模拟的卫星数据
            printf("【卫星数据(模拟)】\n");
            printf("  卫星数: %d  HDOP: %.1f\n", 
                   gps_data.satellites, gps_data.hdop);
                   
        } else {
            printf("【状态】等待定位... ✗ (已尝试: %lu次)\n", packet_count);
            printf("【时间】%s\n", gps_data.timestamp);
            
            if (enable_debug) {
                // 显示一些调试信息，即使未定位
                printf("【原始数据】\n");
                if (gnrmc_data.Time_H > 0 || gnrmc_data.Time_M > 0 || gnrmc_data.Time_S > 0) {
                    printf("  接收到时间数据: %02d:%02d:%02d\n", 
                           gnrmc_data.Time_H, gnrmc_data.Time_M, gnrmc_data.Time_S);
                }
                
                // 显示模拟的卫星数据
                printf("【卫星数据(模拟)】\n");
                printf("  卫星数: %d  HDOP: %.1f\n", 
                       gps_data.satellites, gps_data.hdop);
            }
        }
        
        printf("==========================================\n");
        last_status = gps_data.fix;
    }
}

/**
 * @brief GPS显示主函数
 */
void vendor_gps_display_demo(void) {
    printf("Starting Vendor GPS display demo...\n");
    
    // 设置调试日志级别
    vendor_gps_set_debug(enable_debug);
    printf("调试模式: %s\n", enable_debug ? "开启" : "关闭");
    
    // 初始化GPS
    bool gps_init_success = vendor_gps_init(GPS_UART_ID, GPS_BAUD_RATE, GPS_TX_PIN, GPS_RX_PIN, GPS_FORCE_PIN);
    if (!gps_init_success) {
        printf("GPS初始化失败，检查连接后重试\n");
        return;
    }
    
    printf("GPS初始化成功，开始接收数据\n");
    printf("UART%d 引脚: TX=%d, RX=%d, 波特率=%d\n", 
           GPS_UART_ID, GPS_TX_PIN, GPS_RX_PIN, GPS_BAUD_RATE);
    
    // 发送设置命令 - 配置NMEA输出
    printf("发送NMEA输出配置命令...\n");
    vendor_gps_send_command("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    sleep_ms(100);
    vendor_gps_send_command("$PMTK220,1000");
    sleep_ms(500);
    
    // 为随机数生成器设置种子 (用于模拟卫星信号图)
    srand(time_us_32());
    
    // 绘制UI框架
    draw_gps_ui_frame();
    
    // 立即尝试获取GPS数据
    printf("\n初始状态...\n");
    bool got_data = update_gps_data_from_module();
    print_gps_debug_info(true); // 强制打印第一条数据
    update_gps_display();
    
    if (!got_data) {
        printf("警告：初始化时未能获取到有效GPS数据，请检查GPS模块连接\n");
    }
    
    // 主循环
    uint32_t last_update = 0;
    uint32_t last_blink = 0;
    bool indicator_on = false;
    
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // 每0.5秒尝试更新一次GPS数据（增加采样频率，提高捕获RMC语句的概率）
        if (current_time - last_update >= 500) {
            // 从GPS模块获取数据
            bool got_data = update_gps_data_from_module();
            
            // 打印调试信息到串口
            print_gps_debug_info(false);
            
            // 更新LCD显示
            update_gps_display();
            
            last_update = current_time;
        }
        
        // 绘制GPS活动指示器闪烁 (右上角)
        if (current_time - last_blink >= 500) {
            indicator_on = !indicator_on;
            
            if (indicator_on) {
                st7789_fill_circle(SCREEN_WIDTH - 15, 15, 5, gps_data.fix ? COLOR_GOOD : COLOR_WARNING);
            } else {
                st7789_fill_circle(SCREEN_WIDTH - 15, 15, 5, ST7789_BLUE);
            }
            
            last_blink = current_time;
        }
        
        sleep_ms(50); // 降低CPU占用，但保持较高的响应性
    }
}

/**
 * @brief 主函数入口
 */
int main() {
    // 初始化标准库
    stdio_init_all();
    sleep_ms(2000);  // 等待串口稳定
    
    printf("\n=== Vendor GPS LCD Display Demo ===\n");
    
    // 初始化ST7789
    st7789_config_t config = {
        .spi_inst    = spi0,
        .spi_speed_hz = 40 * 1000 * 1000, // 40MHz
        .pin_din     = 19,
        .pin_sck     = 18,
        .pin_cs      = 17,
        .pin_dc      = 20,
        .pin_reset   = 15,
        .pin_bl      = 10,
        .width       = SCREEN_WIDTH,
        .height      = SCREEN_HEIGHT,
        .rotation    = 0
    };
    
    st7789_init(&config);
    st7789_set_rotation(2); // 竖屏模式，旋转90度
    
    // 打开显示屏背光
    printf("打开显示屏背光...\n");
    st7789_set_backlight(true);
    sleep_ms(500);
    
    // 开始GPS显示演示
    vendor_gps_display_demo();
    
    return 0;
} 