/**
 * @file vendor_gps_example.c
 * @brief L76X GPS模块使用示例 - 厂商代码版本（优化日志输出）
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "gps/vendor_gps_parser.h"

// 配置参数
#define GPS_UART_ID    0          // 使用UART0
#define GPS_TX_PIN     0          // GPIO0 (UART0 TX)
#define GPS_RX_PIN     1          // GPIO1 (UART0 RX)
#define GPS_FORCE_PIN  4          // GPIO4 (FORCE pin)
#define GPS_BAUD_RATE  115200     // GPS模块波特率设置为115200

// 主要GPS数据
static GNRMC gps_data = {0};
static uint32_t packet_count = 0;
static bool enable_debug = false;  // 是否显示详细调试信息

/**
 * @brief Main function
 */
int main() {
    // 初始化标准库 (设置UART等)
    stdio_init_all();
    sleep_ms(2000);  // 等待UART稳定
    
    printf("\n=== L76X GPS模块测试 - 优化版 ===\n");
    printf("UART%d 引脚: TX=%d, RX=%d, 波特率=%d\n", 
           GPS_UART_ID, GPS_TX_PIN, GPS_RX_PIN, GPS_BAUD_RATE);
    
    // 设置调试日志级别
    vendor_gps_set_debug(enable_debug);
    
    // 初始化GPS
    vendor_gps_init(GPS_UART_ID, GPS_BAUD_RATE, GPS_TX_PIN, GPS_RX_PIN, GPS_FORCE_PIN);
    printf("GPS初始化完成，开始接收数据...\n\n");
    
    // 发送设置命令 - 配置NMEA输出
    vendor_gps_send_command("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    vendor_gps_send_command("$PMTK220,1000");
    
    // 主循环
    while (true) {
        // 获取GPS数据 - 使用厂商代码
        gps_data = vendor_gps_get_gnrmc();
        packet_count++;
        
        // 只在状态变化时或每10个包打印一次状态信息
        static bool last_status = false;
        bool status_changed = (last_status != (gps_data.Status == 1));
        bool should_print = status_changed || (packet_count % 10 == 0);
        
        if (should_print) {
            printf("\n[数据包 #%lu] ", packet_count);
            
            if (gps_data.Status) {
                printf("GPS已定位 ✓\n");
                
                // 打印时间和日期
                printf("时间: %02d:%02d:%02d  日期: %s\n", 
                       gps_data.Time_H, gps_data.Time_M, gps_data.Time_S, 
                       gps_data.Date[0] ? gps_data.Date : "未知");
                
                // 打印原始NMEA格式和转换后的十进制度格式
                printf("纬度: %.6f%c → %.6f°\n", 
                       gps_data.Lat_Raw, gps_data.Lat_area, gps_data.Lat);
                printf("经度: %.6f%c → %.6f°\n", 
                       gps_data.Lon_Raw, gps_data.Lon_area, gps_data.Lon);
                
                // 仅在有效定位时显示速度和航向
                if (gps_data.Speed > 0 || gps_data.Course > 0) {
                    printf("速度: %.1f km/h  航向: %.1f°\n", 
                           gps_data.Speed, gps_data.Course);
                }
                
                // 坐标转换
                Coordinates google_coords = vendor_gps_get_google_coordinates();
                Coordinates baidu_coords = vendor_gps_get_baidu_coordinates();
                
                // 只有在真正需要时才显示转换后的坐标
                if (enable_debug) {
                    printf("\n坐标转换结果:\n");
                    printf("Google地图: %.6f, %.6f\n", 
                           google_coords.Lat, google_coords.Lon);
                    printf("百度地图: %.6f, %.6f\n", 
                           baidu_coords.Lat, baidu_coords.Lon);
                }
                
                // 输出百度地图链接 - 这个总是有用的
                printf("百度地图: https://api.map.baidu.com/marker?location=%.6f,%.6f&title=GPS&content=当前位置&output=html\n",
                       baidu_coords.Lat, baidu_coords.Lon);
            } else {
                printf("等待定位... ✗\n");
                
                // 只在调试模式打印更多信息
                if (enable_debug) {
                    printf("时间: %02d:%02d:%02d\n", 
                           gps_data.Time_H, gps_data.Time_M, gps_data.Time_S);
                }
            }
            
            last_status = (gps_data.Status == 1);
        }
        
        sleep_ms(1000);  // 每秒更新一次
    }
    
    return 0;
} 