/**
 * @file vendor_gps_parser.h
 * @brief 厂商提供的GPS解析功能的移植
 */

#ifndef VENDOR_GPS_PARSER_H
#define VENDOR_GPS_PARSER_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"

// 原始厂商代码的数据结构，增强版
typedef struct {
    // 原始结构
    double Lon;         // 经度（十进制度格式）
    double Lat;         // 纬度（十进制度格式）
    char Lon_area;      // 经度区域 ('E'/'W')
    char Lat_area;      // 纬度区域 ('N'/'S')
    uint8_t Time_H;     // 小时
    uint8_t Time_M;     // 分钟
    uint8_t Time_S;     // 秒
    uint8_t Status;     // 定位状态 (1:定位成功 0:定位失败)
    
    // 增强字段
    double Lon_Raw;     // 原始NMEA格式经度（dddmm.mmmm）
    double Lat_Raw;     // 原始NMEA格式纬度（ddmm.mmmm）
    double Speed;       // 速度（公里/小时）
    double Course;      // 航向（度）
    char Date[11];      // 日期（YYYY-MM-DD格式）
    
    // 新增字段
    double Altitude;    // 海拔高度（米）
} GNRMC;

// 坐标结构体
typedef struct {
    double Lon;         // 经度
    double Lat;         // 纬度
} Coordinates;

/**
 * @brief 设置是否输出详细调试日志
 * @param enable 是否启用
 */
void vendor_gps_set_debug(bool enable);

/**
 * @brief 初始化GPS模块的厂商版本
 * @param uart_id UART ID (0 表示 UART0)
 * @param baud_rate 波特率
 * @param tx_pin TX 引脚
 * @param rx_pin RX 引脚
 * @param force_pin FORCE 引脚 (设置为 -1 表示不使用)
 * @return 初始化是否成功
 */
bool vendor_gps_init(uint uart_id, uint baud_rate, uint tx_pin, uint rx_pin, int force_pin);

/**
 * @brief 向GPS模块发送命令
 * @param data 命令字符串，不需要添加校验和
 */
void vendor_gps_send_command(const char *data);

/**
 * @brief 让GPS模块退出备份模式
 */
void vendor_gps_exit_backup_mode(void);

/**
 * @brief 获取GPS的GNRMC数据
 * @return GNRMC 数据结构
 */
GNRMC vendor_gps_get_gnrmc(void);

/**
 * @brief 获取百度地图格式的GPS坐标
 * @return 百度地图坐标
 */
Coordinates vendor_gps_get_baidu_coordinates(void);

/**
 * @brief 获取谷歌地图格式的GPS坐标
 * @return 谷歌地图坐标
 */
Coordinates vendor_gps_get_google_coordinates(void);

#endif /* VENDOR_GPS_PARSER_H */ 