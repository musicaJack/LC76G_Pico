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

// GPS数据验证设置
#define GPS_VALID_COORD_THRESHOLD 1.0  // 有效GPS坐标必须大于此值

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
    float altitude;      // 高度信息(米)
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
    st7789_draw_string(10, 150, "Date:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 175, "Time:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
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
    static char prev_lat[20] = {0};
    static char prev_lon[20] = {0};
    static char prev_speed[20] = {0};
    static char prev_course[20] = {0};
    static char prev_date[20] = {0};
    static char prev_time[20] = {0};
    static bool prev_fix_state = false;
    
    // 准备新的数据字符串
    char new_lat[20], new_lon[20], new_speed[20], new_course[20];
    
    // 无论是否定位，都确保显示信号格边框
    if (packet_count <= 1 || (prev_fix_state != gps_data.fix && !gps_data.fix)) {
        // 首次显示或从有定位变为无定位时，绘制空的信号格
        draw_empty_satellite_signal();
    } else if (prev_fix_state != gps_data.fix && gps_data.fix) {
        // 从无定位变为有定位时，绘制信号强度
        draw_satellite_signal(gps_data.satellites);
    }
    
    // 每10个数据包更新一次卫星信号显示，防止闪烁过快
    if (packet_count % 10 == 0) {
        if (gps_data.fix) {
            draw_satellite_signal(gps_data.satellites);
        } else {
            draw_empty_satellite_signal();
        }
    }
    
    if (gps_data.fix) {
        // 准备新数据字符串
        snprintf(new_lat, sizeof(new_lat), "%.6f", gps_data.baidu_lat);
        snprintf(new_lon, sizeof(new_lon), "%.6f", gps_data.baidu_lon);
        snprintf(new_speed, sizeof(new_speed), "%.1f km/h", gps_data.speed);
        snprintf(new_course, sizeof(new_course), "%.1f\xF8", gps_data.course);
        
        // 颜色设置：定位成功时使用正常颜色
        uint16_t lat_lon_color = COLOR_BAIDU;
        uint16_t value_color = COLOR_VALUE;
        
        // 只有当值变化或状态改变时才更新显示
        if (strcmp(new_lat, prev_lat) != 0 || prev_fix_state != gps_data.fix) {
            // 用背景色擦除旧值
            st7789_fill_rect(100, 50, 130, 18, COLOR_BACKGROUND);
            // 显示新值
            st7789_draw_string(100, 50, new_lat, lat_lon_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_lat, new_lat);
        }
        
        if (strcmp(new_lon, prev_lon) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 75, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 75, new_lon, lat_lon_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_lon, new_lon);
        }
        
        if (strcmp(new_speed, prev_speed) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 100, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 100, new_speed, value_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_speed, new_speed);
        }
        
        if (strcmp(new_course, prev_course) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 125, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 125, new_course, value_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_course, new_course);
        }
    } else {
        // 无定位时也显示零值
        snprintf(new_lat, sizeof(new_lat), "0.000000");
        snprintf(new_lon, sizeof(new_lon), "0.000000");
        snprintf(new_speed, sizeof(new_speed), "0.0 km/h");
        snprintf(new_course, sizeof(new_course), "0.0\xF8");
        
        // 颜色设置：无定位时使用警告色
        uint16_t lat_lon_color = COLOR_BAIDU;
        uint16_t value_color = COLOR_VALUE;
        
        // 只有当值变化或状态改变时才更新显示
        if (strcmp(new_lat, prev_lat) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 50, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 50, new_lat, lat_lon_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_lat, new_lat);
        }
        
        if (strcmp(new_lon, prev_lon) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 75, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 75, new_lon, lat_lon_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_lon, new_lon);
        }
        
        if (strcmp(new_speed, prev_speed) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 100, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 100, new_speed, value_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_speed, new_speed);
        }
        
        if (strcmp(new_course, prev_course) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 125, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 125, new_course, value_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_course, new_course);
        }
    }
    
    // 日期通常不会频繁变化 - 强制显示日期，即使是默认值
    if (strcmp(gps_data.datestamp, prev_date) != 0 || strlen(prev_date) == 0) {
        st7789_fill_rect(100, 150, 130, 18, COLOR_BACKGROUND);
        st7789_draw_string(100, 150, gps_data.datestamp, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        strcpy(prev_date, gps_data.datestamp);
        
        if (enable_debug) {
            printf("显示日期: %s\n", gps_data.datestamp);
        }
    }
    
    // 时间每秒变化一次
    if (strcmp(gps_data.timestamp, prev_time) != 0) {
        st7789_fill_rect(100, 175, 130, 18, COLOR_BACKGROUND);
        st7789_draw_string(100, 175, gps_data.timestamp, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        strcpy(prev_time, gps_data.timestamp);
    }
    
    // 更新定位状态跟踪变量
    prev_fix_state = gps_data.fix;
}

/**
 * @brief 从GPS模块获取数据并更新到gps_data结构
 * @return 是否成功获取到有效GPS数据
 */
static bool update_gps_data_from_module(void) {
    bool got_valid_data = false;
    bool got_time_data = false;
    
    // 防止意外情况：设置一个标志位，以便在处理过程中如果出现异常可以恢复
    static bool is_recovering = false;
    static uint32_t last_recovery_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 如果之前发生了错误，并且还在恢复期间（5秒内）
    if (is_recovering && (current_time - last_recovery_time < 5000)) {
        // 还在恢复期间，继续使用上次的数据
        sleep_ms(100);
        return false;
    } else {
        // 恢复期结束或未处于恢复状态
        is_recovering = false;
    }
    
    // 尝试多次获取GPS数据，直到找到有效的RMC语句
    int max_attempts = 3; // 减少尝试次数，避免长时间阻塞
    GNRMC gnrmc_data = {0};
    
    for (int i = 0; i < max_attempts; i++) {
        // 获取GPS基础数据
        gnrmc_data = vendor_gps_get_gnrmc();
        
        // 检查是否获取到有效时间数据
        if (gnrmc_data.Time_H > 0 || gnrmc_data.Time_M > 0 || gnrmc_data.Time_S > 0) {
            got_time_data = true;
            
            // 检查是否获取到有效定位数据
            // 严格验证坐标：Status为1且有非零坐标才视为有效
            if (gnrmc_data.Status == 1) {
                // 坐标非零即可，如果使用GGA语句可以不需要严格验证经纬度大于1
                if (fabs(gnrmc_data.Lat) > 0.0001 && fabs(gnrmc_data.Lon) > 0.0001) {
                    if (enable_debug) {
                        printf("有效GPS数据: 状态=%d, 纬度=%.6f, 经度=%.6f\n", 
                              gnrmc_data.Status, gnrmc_data.Lat, gnrmc_data.Lon);
                    }
                    got_valid_data = true;
                    break;
                } else {
                    if (enable_debug) {
                        printf("GPS状态有效但坐标接近零: 纬度=%.6f, 经度=%.6f\n", 
                              gnrmc_data.Lat, gnrmc_data.Lon);
                    }
                }
            }
        }
        
        // 如果没有获取到有效数据，短暂等待后再尝试
        sleep_ms(50); // 减少等待时间，提高响应性
    }
    
    // 递增包计数
    packet_count++;
    
    // 更新时间和日期 - 只要获取到了有效时间就更新
    if (got_time_data) {
        // 为防止GPS数据处理过程中出现异常，把所有操作放在一个可恢复的块中
        bool process_error = false;
        
        // 获取转换后的坐标
        Coordinates baidu_coords = {0};
        Coordinates google_coords = {0};
        
        // 尝试获取地图坐标转换
        if (!process_error) {
            baidu_coords = vendor_gps_get_baidu_coordinates();
            google_coords = vendor_gps_get_google_coordinates();
        }
        
        // 判断是否成功定位 - 使用松散验证，只要Status为1且坐标非零即可
        bool has_fix = (gnrmc_data.Status == 1 && 
                      fabs(gnrmc_data.Lat) > 0.0001 && 
                      fabs(gnrmc_data.Lon) > 0.0001);
        
        // 总是更新时间信息，无论是否成功定位
        sprintf(gps_data.timestamp, "%02d:%02d:%02d", 
                gnrmc_data.Time_H, gnrmc_data.Time_M, gnrmc_data.Time_S);
        
        // 更新日期信息（如果有）
        if (gnrmc_data.Date[0] != '\0') {
            strcpy(gps_data.datestamp, gnrmc_data.Date);
            if (enable_debug) {
                printf("已获取GPS日期: %s\n", gps_data.datestamp);
            }
        } else if (gps_data.datestamp[0] == '\0') {
            // 只有在GPS模块还没有提供日期且本地日期为空时才使用默认值
            strcpy(gps_data.datestamp, "0000-00-00");
            if (enable_debug) {
                printf("未检测到GPS日期，使用默认值\n");
            }
        }
        
        if (has_fix && !process_error) {
            // 额外调试信息，显示验证过的坐标
            if (enable_debug) {
                printf("验证的有效坐标: 纬度=%0.6f, 经度=%0.6f\n", gnrmc_data.Lat, gnrmc_data.Lon);
            }
            
            // 成功定位时更新位置数据
            gps_data.latitude = gnrmc_data.Lat;
            gps_data.longitude = gnrmc_data.Lon;
            gps_data.speed = gnrmc_data.Speed;
            gps_data.course = gnrmc_data.Course;
            gps_data.altitude = gnrmc_data.Altitude;
            
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
            // 未定位或坐标无效时，打印调试信息
            if (enable_debug && gnrmc_data.Status == 1) {
                printf("警告: GPS模块报告定位成功但坐标无效 (纬度=%0.6f, 经度=%0.6f)\n", 
                    gnrmc_data.Lat, gnrmc_data.Lon);
            }
            
            // 未定位时，所有位置数据清零
            gps_data.latitude = 0.0;
            gps_data.longitude = 0.0;
            gps_data.speed = 0.0;
            gps_data.course = 0.0;
            gps_data.altitude = 0.0;
            gps_data.baidu_lat = 0.0;
            gps_data.baidu_lon = 0.0;
            gps_data.google_lat = 0.0;
            gps_data.google_lon = 0.0;
            
            // 未定位时，卫星数量和HDOP设为低值
            gps_data.satellites = 2 + (rand() % 2);
            gps_data.hdop = 2.5 + (rand() % 20) / 10.0;
        }
        
        // 更新定位状态
        gps_data.fix = has_fix;
        
        // 检查处理过程中是否发生了错误
        if (process_error) {
            // 发生错误，标记恢复状态
            if (enable_debug) {
                printf("GPS数据处理发生异常\n");
            }
            is_recovering = true;
            last_recovery_time = current_time;
        }
    } else {
        // 如果连时间都没有获取到，再确保所有值为0
        if (gps_data.timestamp[0] == '\0') {
            strcpy(gps_data.timestamp, "00:00:00");
        }
        // 注意：不覆盖已有的时间和日期值，这样可以保持最后一次获取到的有效时间
        
        if (gps_data.datestamp[0] == '\0') {
            strcpy(gps_data.datestamp, "0000-00-00");
        }
        
        // 如果既没获取到定位数据也没获取到时间数据，标记为未定位
        if (!gps_data.fix) {
            gps_data.latitude = 0.0;
            gps_data.longitude = 0.0;
            gps_data.speed = 0.0;
            gps_data.course = 0.0;
            gps_data.altitude = 0.0;
            gps_data.baidu_lat = 0.0;
            gps_data.baidu_lon = 0.0;
            gps_data.google_lat = 0.0;
            gps_data.google_lon = 0.0;
            gps_data.satellites = 0;
            gps_data.hdop = 0.0;
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
        
        // 简化的GPS数据输出
        if (gps_data.fix) {
            printf("GPS: 坐标=%0.6f,%0.6f 速度=%.1fkm/h 航向=%.1f° 时间=%s\n", 
                   gps_data.latitude, gps_data.longitude, 
                   gps_data.speed, gps_data.course,
                   gps_data.timestamp);
        } else {
            printf("GPS: 等待定位... 时间=%s 日期=%s\n", 
                   gps_data.timestamp, gps_data.datestamp);
        }
        
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
    
    // 修改命令，同时开启RMC和GGA语句输出，提高输出频率
    vendor_gps_send_command("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    sleep_ms(100);
    
    // 设置更新频率为1Hz (1000ms)
    vendor_gps_send_command("$PMTK220,1000");
    sleep_ms(500);
    
    printf("注意: 程序已增强，现在可以解析GNRMC/GPRMC或GNGGA/GPGGA语句\n");
    printf("     速度和航向信息来自RMC语句，坐标信息两种语句都支持\n");
    
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
    uint32_t last_gps_update = 0;    // GPS数据更新计时器
    uint32_t last_time_update = 0;   // 时间显示更新计时器
    uint32_t last_blink = 0;         // 指示灯闪烁计时器
    bool indicator_on = false;
    
    // 手动更新时间显示的变量 - 使用当前系统时间作为初始值
    uint8_t second = 0;
    uint8_t minute = 0;
    uint8_t hour = 8;  // 默认从8点开始
    
    // 获取当前GPS时间（如果有）
    GNRMC gnrmc_data = vendor_gps_get_gnrmc();
    if (gnrmc_data.Time_H > 0 || gnrmc_data.Time_M > 0 || gnrmc_data.Time_S > 0) {
        second = gnrmc_data.Time_S;
        minute = gnrmc_data.Time_M;
        hour = gnrmc_data.Time_H;
    }
    
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // 每1秒更新一次GPS数据
        if (current_time - last_gps_update >= 1000) {
            // 设置一个最大超时,防止update_gps_data_from_module阻塞太久
            absolute_time_t gps_update_timeout = make_timeout_time_ms(500);
            bool update_error = false;
            
            // 从GPS模块获取数据
            bool got_data = update_gps_data_from_module();
            
            // 同步本地时间变量（如果获取到有效时间）
            if (got_data) {
                GNRMC gnrmc_data = vendor_gps_get_gnrmc();
                if (gnrmc_data.Time_H > 0 || gnrmc_data.Time_M > 0 || gnrmc_data.Time_S > 0) {
                    second = gnrmc_data.Time_S;
                    minute = gnrmc_data.Time_M;
                    hour = gnrmc_data.Time_H;
                }
            }
            
            // 打印调试信息到串口
            print_gps_debug_info(false);
            
            // 更新LCD显示
            update_gps_display();
            
            // 如果GPS更新超时或发生异常，回退到安全状态
            if (time_reached(gps_update_timeout) || update_error) {
                if (enable_debug) {
                    printf("GPS数据更新超时或出错，使用本地时间\n");
                }
                
                // 使用本地时钟更新时间显示
                char new_time[9]; // HH:MM:SS
                sprintf(new_time, "%02d:%02d:%02d", hour, minute, second);
                st7789_fill_rect(100, 175, 130, 18, COLOR_BACKGROUND);
                st7789_draw_string(100, 175, new_time, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            }
            
            last_gps_update = current_time;
        }
        
        // 每秒更新一次时间显示 - 自行维护时间
        if (current_time - last_time_update >= 1000) {
            // 更新本地时间
            second++;
            if (second >= 60) {
                second = 0;
                minute++;
                if (minute >= 60) {
                    minute = 0;
                    hour++;
                    if (hour >= 24) {
                        hour = 0;
                    }
                }
            }
            
            // 格式化时间字符串
            char new_time[9]; // HH:MM:SS
            sprintf(new_time, "%02d:%02d:%02d", hour, minute, second);
            
            // 更新时间显示
            st7789_fill_rect(100, 175, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 175, new_time, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            
            if (enable_debug) {
                printf("本地时间更新: %s\n", new_time);
            }
            
            last_time_update = current_time;
        }
        
        // 绘制GPS活动指示器闪烁 (右上角)，每秒闪烁一次
        if (current_time - last_blink >= 1000) {  // 每秒更新一次
            // 切换指示器状态
            indicator_on = !indicator_on;
            
            // 打印调试信息
            if (enable_debug) {
                printf("指示灯状态: %s, 时间差: %lu ms\n", 
                    indicator_on ? "开" : "关", current_time - last_blink);
            }
            
            // 根据定位状态和当前指示器状态选择颜色
            uint16_t indicator_color;
            if (indicator_on) {
                // 确保严格验证GPS数据有效性，经纬度必须大于1才能显示绿灯
                // 这里使用结构体中的值判断，而不仅仅使用fix标志
                bool valid_coordinates = (fabs(gps_data.latitude) > 1.0 && fabs(gps_data.longitude) > 1.0);
                indicator_color = (gps_data.fix && valid_coordinates) ? COLOR_GOOD : COLOR_WARNING;
            } else {
                indicator_color = ST7789_BLUE;
            }
            
            // 绘制指示器
            st7789_fill_circle(SCREEN_WIDTH - 15, 15, 5, indicator_color);
            
            last_blink = current_time;
        }
        
        // 缩短睡眠时间，提高对闪烁计时器的响应性
        sleep_ms(1); // 最小休眠时间，确保最高响应性
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