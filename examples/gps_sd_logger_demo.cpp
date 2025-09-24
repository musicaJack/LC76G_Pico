/**
 * @file gps_sd_logger_demo.cpp
 * @brief GPS坐标SD卡日志记录演示程序
 * @version 1.0.0
 * 
 * 功能演示:
 * - GPS数据读取和坐标转换
 * - 内存优化的批量写入SD卡
 * - 高德地图API兼容格式
 * - 实时内存使用监控
 * - 错误处理和恢复
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

// 项目头文件
#include "gps/lc76g_i2c_adaptor.h"
#include "gps/gps_logger.hpp"
#include "pin_config.hpp"

// =============================================================================
// 配置参数
// =============================================================================

// GPS配置
#define GPS_UPDATE_INTERVAL_MS    1000    // GPS更新间隔 (1秒)
#define GPS_READ_TIMEOUT_MS       5000    // GPS读取超时 (5秒)

// 日志配置
#define LOG_FLUSH_INTERVAL_MS     10000   // 日志刷新间隔 (10秒)
#define LOG_STATS_INTERVAL_MS     30000   // 统计信息间隔 (30秒)

// 内存监控
#define MEMORY_CHECK_INTERVAL_MS  5000    // 内存检查间隔 (5秒)

// =============================================================================
// 全局变量
// =============================================================================

static GPS::GPSLogger* g_gps_logger = nullptr;
static LC76G_GPS_Data g_last_gps_data = {0};
static uint64_t g_last_gps_time = 0;
static uint64_t g_last_log_flush = 0;
static uint64_t g_last_stats_time = 0;
static uint64_t g_last_memory_check = 0;
static uint32_t g_total_records = 0;
static uint32_t g_failed_records = 0;

// =============================================================================
// 函数声明
// =============================================================================

static bool initialize_gps();
static bool initialize_sd_logger();
static void process_gps_data();
static void check_log_flush();
static void print_statistics();
static void check_memory_usage();
static void print_startup_info();
static void print_gps_data(const LC76G_GPS_Data& gps_data);

// =============================================================================
// 主程序
// =============================================================================

int main() {
    // 初始化标准库
    stdio_init_all();
    
    // 等待串口连接
    sleep_ms(2000);
    
    printf("\n=== GPS SD卡日志记录演示程序 ===\n");
    printf("版本: 1.0.0\n");
    printf("目标: 内存优化的GPS坐标记录\n\n");
    
    // 打印启动信息
    print_startup_info();
    
    // 初始化GPS模块
    printf("[初始化] 正在初始化GPS模块...\n");
    if (!initialize_gps()) {
        printf("[错误] GPS模块初始化失败\n");
        return -1;
    }
    printf("[成功] GPS模块初始化完成\n");
    
    // 初始化SD卡日志记录器
    printf("[初始化] 正在初始化SD卡日志记录器...\n");
    if (!initialize_sd_logger()) {
        printf("[错误] SD卡日志记录器初始化失败\n");
        return -1;
    }
    printf("[成功] SD卡日志记录器初始化完成\n");
    
    // 打印配置信息
    printf("\n=== 配置信息 ===\n");
    printf("GPS更新间隔: %d 毫秒\n", GPS_UPDATE_INTERVAL_MS);
    printf("日志刷新间隔: %d 毫秒\n", LOG_FLUSH_INTERVAL_MS);
    printf("统计信息间隔: %d 毫秒\n", LOG_STATS_INTERVAL_MS);
    printf("内存检查间隔: %d 毫秒\n", MEMORY_CHECK_INTERVAL_MS);
    printf("当前日志文件: %s\n", g_gps_logger->get_current_log_file().c_str());
    printf("========================\n\n");
    
    // 主循环
    printf("[开始] 进入主循环，开始GPS数据记录...\n");
    uint64_t loop_count = 0;
    
    while (true) {
        uint64_t current_time = to_ms_since_boot(get_absolute_time());
        
        // 处理GPS数据
        process_gps_data();
        
        // 检查日志刷新
        check_log_flush();
        
        // 打印统计信息
        if (current_time - g_last_stats_time >= LOG_STATS_INTERVAL_MS) {
            print_statistics();
            g_last_stats_time = current_time;
        }
        
        // 检查内存使用
        if (current_time - g_last_memory_check >= MEMORY_CHECK_INTERVAL_MS) {
            check_memory_usage();
            g_last_memory_check = current_time;
        }
        
        // 循环计数
        loop_count++;
        if (loop_count % 100 == 0) {
            printf("[状态] 主循环运行 %llu 次，记录 %u 条，失败 %u 条\n", 
                   loop_count, g_total_records, g_failed_records);
        }
        
        // 短暂休眠
        sleep_ms(100);
    }
    
    return 0;
}

// =============================================================================
// 初始化函数
// =============================================================================

static bool initialize_gps() {
    // 初始化LC76G GPS模块
    bool success = lc76g_i2c_init(
        GPS_I2C_INST,           // I2C实例
        GPS_PIN_SDA,            // SDA引脚
        GPS_PIN_SCL,            // SCL引脚
        GPS_I2C_SPEED,          // I2C速度
        GPS_FORCE_PIN           // FORCE引脚
    );
    
    if (!success) {
        printf("[GPS] I2C初始化失败\n");
        return false;
    }
    
    // 启用调试输出
    lc76g_set_debug(true);
    
    // 等待GPS模块稳定
    printf("[GPS] 等待GPS模块稳定...\n");
    sleep_ms(3000);
    
    return true;
}

static bool initialize_sd_logger() {
    // 配置SD卡 (使用默认配置，避免引脚冲突)
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
    g_gps_logger = new GPS::GPSLogger(sd_config, log_config);
    
    if (!g_gps_logger->initialize()) {
        printf("[SD Logger] 初始化失败\n");
        delete g_gps_logger;
        g_gps_logger = nullptr;
        return false;
    }
    
    return true;
}

// =============================================================================
// 数据处理函数
// =============================================================================

static void process_gps_data() {
    uint64_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 检查GPS更新间隔
    if (current_time - g_last_gps_time < GPS_UPDATE_INTERVAL_MS) {
        return;
    }
    
    // 读取GPS数据
    LC76G_GPS_Data gps_data;
    if (lc76g_read_gps_data(&gps_data)) {
        // 检查数据有效性
        if (gps_data.Status) {
            // 打印GPS数据
            print_gps_data(gps_data);
            
            // 记录到SD卡
            if (g_gps_logger && g_gps_logger->log_gps_data(gps_data)) {
                g_total_records++;
                g_last_gps_data = gps_data;
            } else {
                g_failed_records++;
                printf("[警告] GPS数据记录失败\n");
            }
        } else {
            printf("[GPS] 等待有效定位... (卫星: %d)\n", gps_data.Satellites);
        }
    } else {
        printf("[GPS] 读取GPS数据失败\n");
    }
    
    g_last_gps_time = current_time;
}

static void check_log_flush() {
    uint64_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 检查日志刷新间隔
    if (current_time - g_last_log_flush >= LOG_FLUSH_INTERVAL_MS) {
        if (g_gps_logger) {
            if (g_gps_logger->flush_buffer()) {
                printf("[日志] 缓冲区已刷新\n");
            } else {
                printf("[警告] 缓冲区刷新失败\n");
            }
        }
        g_last_log_flush = current_time;
    }
}

// =============================================================================
// 信息输出函数
// =============================================================================

static void print_startup_info() {
    printf("=== 硬件配置 ===\n");
    printf("GPS I2C: I2C%d, SDA: %d, SCL: %d\n", 
           GPS_I2C_INST == i2c0 ? 0 : 1, GPS_PIN_SDA, GPS_PIN_SCL);
    printf("SD卡 SPI: SPI1, MISO: 11, MOSI: 12, SCK: 10, CS: 13\n");
    printf("FORCE引脚: %d\n", GPS_FORCE_PIN);
    printf("================\n\n");
}

static void print_gps_data(const LC76G_GPS_Data& gps_data) {
    printf("[GPS] 坐标: %.6f,%.6f | 时间: %02d:%02d:%02d | 卫星: %d | 速度: %.1f km/h\n",
           gps_data.Lon, gps_data.Lat,
           gps_data.Time_H, gps_data.Time_M, gps_data.Time_S,
           gps_data.Satellites, gps_data.Speed);
}

static void print_statistics() {
    printf("\n=== 统计信息 ===\n");
    printf("总记录数: %u\n", g_total_records);
    printf("失败记录: %u\n", g_failed_records);
    printf("成功率: %.1f%%\n", g_total_records > 0 ? 
           (float)(g_total_records - g_failed_records) * 100.0f / g_total_records : 0.0f);
    
    if (g_gps_logger) {
        printf("当前日志文件: %s\n", g_gps_logger->get_current_log_file().c_str());
        printf("%s", g_gps_logger->get_log_statistics().c_str());
    }
    printf("================\n\n");
}

static void check_memory_usage() {
    if (g_gps_logger) {
        printf("[内存] %s", g_gps_logger->get_memory_usage().c_str());
    }
}
