/**
 * @file lc76g_i2c_adaptor.h
 * @brief LC76G GPS模块I2C适配器 - 基于厂商demo移植到Pico平台
 * 
 * 基于厂商提供的LC76G I2C demo代码，移植到Pico平台
 * 实现完整的I2C通信协议和GPS数据解析功能
 */

#ifndef LC76G_I2C_ADAPTOR_H
#define LC76G_I2C_ADAPTOR_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// LC76G I2C寄存器定义 (基于厂商demo)
// =============================================================================

#include "pin_config.hpp"

#define TIME_OUT     200
#define RETRY_TIME   20
#define QL_CRCW_ADDR I2C_ADDRESS_CR_OR_CW  // 控制/读写地址
#define QL_RD_ADDR   I2C_ADDRESS_R         // 读地址
#define QL_WR_ADDR   I2C_ADDRESS_W         // 写地址

#define QL_RW_DATA_LENGTH_SIZE 4
#define QL_MAX_DATA_LENGTH 4096

// 模块寄存器
#define QL_CR_REG    0xaa510008  // 控制寄存器
#define QL_RD_REG    0xaa512000  // 读数据寄存器
#define QL_CW_REG    0xaa510004  // 控制写寄存器
#define QL_WR_REG    0xaa531000  // 写数据寄存器

// 寄存器长度
#define QL_CR_LEN    0x00000004
#define QL_CW_LEN    0x00000004

// =============================================================================
// 命令解析相关定义
// =============================================================================

enum DECODE_ERROR {
    Format_Error = 1,
    CheckSum_Error = 2,
    Data_Error = 3,
    No_Error = 0
};

enum COMMAND_RSP_GET_ERROR {
    WAITING  = 1,
    NOGET    = 2,
    GET      = 0 
};

// GPS命令上下文结构
typedef struct {
    char param[40][30];
    int32_t param_num;
    int32_t checksum;
} Ql_gnss_command_contx_TypeDef;

// GPS命令处理结构
typedef struct {
    uint8_t cmd_buf[100];
    uint8_t ex_rsp_buf[100];
    Ql_gnss_command_contx_TypeDef cmd_par;
    uint8_t rsp_buf[100];
    uint8_t retry_time; 
    uint8_t get_rsp_flag;
} Ql_GNSS_Command_TypeDef;

// =============================================================================
// GPS数据结构
// =============================================================================

// LC76G GPS数据结构
typedef struct {
    // 基本定位数据
    double Lon;         // 经度 (十进制度格式)
    double Lat;         // 纬度 (十进制度格式)
    char Lon_area;      // 经度区域 ('E'/'W')
    char Lat_area;      // 纬度区域 ('N'/'S')
    uint8_t Time_H;     // 小时
    uint8_t Time_M;     // 分钟
    uint8_t Time_S;     // 秒
    uint8_t Status;     // 定位状态 (1:定位成功 0:定位失败)
    
    // 增强字段
    double Lon_Raw;     // 原始NMEA格式经度 (dddmm.mmmm)
    double Lat_Raw;     // 原始NMEA格式纬度 (ddmm.mmmm)
    double Speed;       // 速度 (公里/小时)
    double Course;      // 航向 (度)
    char Date[11];      // 日期 (YYYY-MM-DD格式)
    double Altitude;    // 海拔 (米)
    
    // LC76G特定字段
    uint8_t Quality;    // GPS质量 (0=无效, 1=GPS SPS, 2=差分/SBAS)
    uint8_t Satellites; // 使用的卫星数量
    double HDOP;        // 水平精度因子
    double PDOP;        // 位置精度因子
    double VDOP;        // 垂直精度因子
    char Mode;          // 模式指示 (A=自主, D=差分, N=无定位)
    char NavStatus;     // 导航状态 (V=无效, A=有效)
} LC76G_GPS_Data;

// 坐标结构
typedef struct {
    double Lon;         // 经度
    double Lat;         // 纬度
} Coordinates;

// =============================================================================
// I2C适配器函数声明
// =============================================================================

/**
 * @brief 初始化LC76G I2C适配器
 * @param i2c_inst I2C实例
 * @param sda_pin SDA引脚
 * @param scl_pin SCL引脚
 * @param i2c_speed I2C速度
 * @param force_pin FORCE引脚 (可选，设为-1表示不使用)
 * @return 初始化是否成功
 */
bool lc76g_i2c_init(i2c_inst_t *i2c_inst, uint sda_pin, uint scl_pin, uint i2c_speed, int force_pin);

/**
 * @brief 发送命令到LC76G模块
 * @param cmd_buf 命令缓冲区
 * @param cmd_len 命令长度
 * @return 发送是否成功
 */
bool lc76g_send_command(const char *cmd_buf, int cmd_len);

/**
 * @brief 发送命令并获取响应
 * @param cmd_buf 命令缓冲区
 * @param expect_rsp 期望的响应前缀
 * @param timeout_ms 超时时间(毫秒)
 * @param info 命令上下文结构指针
 * @return 是否成功获取响应
 */
bool lc76g_send_command_and_get_response(const char *cmd_buf, const char *expect_rsp, 
                                        uint32_t timeout_ms, Ql_gnss_command_contx_TypeDef *info);

/**
 * @brief 读取GPS数据
 * @param gps_data GPS数据结构指针
 * @return 是否成功读取到有效数据
 */
bool lc76g_read_gps_data(LC76G_GPS_Data *gps_data);

/**
 * @brief 设置调试输出
 * @param enable 是否启用调试输出
 */
void lc76g_set_debug(bool enable);

/**
 * @brief 唤醒I2C通信
 * @return 是否成功
 */
bool lc76g_wake_i2c(void);

/**
 * @brief 重置LC76G模块
 */
void lc76g_reset_module(void);

// =============================================================================
// 工具函数声明
// =============================================================================

/**
 * @brief 计算命令校验和
 * @param buffer 缓冲区
 * @param buffer_len 缓冲区长度
 * @return 校验和
 */
int32_t lc76g_get_command_checksum(const char *buffer, int32_t buffer_len);

/**
 * @brief 解析NMEA命令参数
 * @param command 命令字符串
 * @param length 命令长度
 * @param contx 命令上下文结构
 * @return 解析结果
 */
uint8_t lc76g_command_get_param(const char *command, int32_t length, Ql_gnss_command_contx_TypeDef *contx);

/**
 * @brief 获取百度地图坐标
 * @return 百度地图坐标
 */
Coordinates lc76g_get_baidu_coordinates(void);

/**
 * @brief 获取Google地图坐标
 * @return Google地图坐标
 */
Coordinates lc76g_get_google_coordinates(void);

#ifdef __cplusplus
}
#endif

#endif /* LC76G_I2C_ADAPTOR_H */
