/**
 * @file gps_logger.hpp
 * @brief GPS坐标日志记录器 - 内存优化的流式SD卡存储
 * @version 2.0.0
 * 
 * 功能特性:
 * - 内存优化的流式写入 (避免大量数据缓存)
 * - 自动按日期创建日志文件 (yyyymmdd_自编号.log)
 * - 支持高德地图API兼容的坐标格式
 * - 自动坐标转换 (WGS84 -> GCJ02)
 * - 时间戳ISO 8601格式
 * - 文件大小限制和自动轮转
 * - 批量写入优化 (减少SD卡写入次数)
 * - 错误处理和恢复机制
 * - 适配RP2040内存限制 (264KB RAM)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include <array>
#include "gps/lc76g_i2c_adaptor.h"
#include "rw_sd.hpp"

namespace GPS {

/**
 * @brief GPS日志记录器类
 */
class GPSLogger {
public:
    /**
     * @brief 日志配置结构 - 内存优化配置
     */
    struct LogConfig {
        std::string log_directory = "/gps_logs";     // 日志目录
        size_t max_file_size = 512 * 1024;          // 最大文件大小 (512KB，减少内存压力)
        size_t max_files_per_day = 20;              // 每天最大文件数
        bool auto_create_directory = true;          // 自动创建目录
        bool enable_coordinate_transform = true;    // 启用坐标转换
        std::string file_extension = ".log";        // 文件扩展名
        
        // 内存优化配置
        size_t buffer_size = 1024;                  // 写入缓冲区大小 (1KB)
        size_t batch_write_count = 10;              // 批量写入条数
        uint32_t write_interval_ms = 5000;          // 批量写入间隔 (5秒)
        bool enable_immediate_write = false;        // 是否立即写入 (调试用)
    };

    /**
     * @brief 坐标数据结构
     */
    struct CoordinateData {
        double longitude;        // 经度 (WGS84)
        double latitude;         // 纬度 (WGS84)
        double longitude_gcj02;  // 经度 (GCJ02 - 高德坐标系)
        double latitude_gcj02;   // 纬度 (GCJ02 - 高德坐标系)
        std::string timestamp;   // ISO 8601时间戳
        uint8_t satellites;      // 卫星数量
        double hdop;            // 水平精度因子
        bool is_valid;          // 数据有效性
    };

private:
    std::unique_ptr<MicroSD::RWSD> sd_card_;
    LogConfig config_;
    std::string current_log_file_;
    size_t current_file_size_;
    uint32_t daily_file_counter_;
    std::string current_date_;
    bool is_initialized_;
    
    // 内存优化的批量写入缓冲区
    std::array<char, 2048> write_buffer_;           // 2KB写入缓冲区
    size_t buffer_used_;                            // 缓冲区已使用大小
    size_t pending_records_;                        // 待写入记录数
    uint64_t last_write_time_;                      // 上次写入时间
    
    // 坐标转换相关
    static constexpr double PI = 3.14159265358979324;
    static constexpr double A = 6378245.0;
    static constexpr double EE = 0.00669342162296594323;

public:
    /**
     * @brief 构造函数
     * @param sd_config SD卡配置
     * @param log_config 日志配置
     */
    GPSLogger(const MicroSD::SPIConfig& sd_config = MicroSD::Config::DEFAULT,
              const LogConfig& log_config = LogConfig{
                  .log_directory = "/gps_logs",
                  .max_file_size = 512 * 1024,
                  .max_files_per_day = 20,
                  .auto_create_directory = true,
                  .enable_coordinate_transform = true,
                  .file_extension = ".log",
                  .buffer_size = 1024,
                  .batch_write_count = 10,
                  .write_interval_ms = 5000,
                  .enable_immediate_write = false
              });

    /**
     * @brief 析构函数
     */
    ~GPSLogger();

    // 禁用拷贝构造和赋值
    GPSLogger(const GPSLogger&) = delete;
    GPSLogger& operator=(const GPSLogger&) = delete;

    // 支持移动语义
    GPSLogger(GPSLogger&& other) noexcept;
    GPSLogger& operator=(GPSLogger&& other) noexcept;

    /**
     * @brief 初始化GPS日志记录器
     * @return 初始化是否成功
     */
    bool initialize();

    /**
     * @brief 检查是否已初始化
     */
    bool is_initialized() const { return is_initialized_; }

    /**
     * @brief 记录GPS数据到SD卡
     * @param gps_data LC76G GPS数据结构
     * @return 记录是否成功
     */
    bool log_gps_data(const LC76G_GPS_Data& gps_data);

    /**
     * @brief 记录坐标数据到SD卡
     * @param coord_data 坐标数据结构
     * @return 记录是否成功
     */
    bool log_coordinate_data(const CoordinateData& coord_data);

    /**
     * @brief 从LC76G GPS数据创建坐标数据
     * @param gps_data LC76G GPS数据结构
     * @return 坐标数据结构
     */
    CoordinateData create_coordinate_data(const LC76G_GPS_Data& gps_data);

    /**
     * @brief 获取当前日志文件路径
     */
    std::string get_current_log_file() const { return current_log_file_; }

    /**
     * @brief 获取日志配置
     */
    const LogConfig& get_config() const { return config_; }

    /**
     * @brief 设置日志配置
     * @param config 新的配置
     */
    void set_config(const LogConfig& config) { config_ = config; }

    /**
     * @brief 获取日志文件列表
     * @return 日志文件路径列表
     */
    std::vector<std::string> get_log_files() const;

    /**
     * @brief 清理旧日志文件
     * @param days_to_keep 保留天数
     * @return 清理的文件数量
     */
    size_t cleanup_old_logs(int days_to_keep = 30);

    /**
     * @brief 获取日志统计信息
     * @return 统计信息字符串
     */
    std::string get_log_statistics() const;

    /**
     * @brief 强制同步数据到SD卡
     * @return 同步是否成功
     */
    bool sync();

    /**
     * @brief 强制刷新缓冲区到SD卡
     * @return 刷新是否成功
     */
    bool flush_buffer();

    /**
     * @brief 检查是否需要批量写入
     * @return 是否需要写入
     */
    bool should_batch_write();

    /**
     * @brief 获取内存使用统计
     * @return 内存使用信息字符串
     */
    std::string get_memory_usage() const;

private:
    /**
     * @brief 生成日志文件名
     * @param date 日期字符串 (YYYYMMDD)
     * @param counter 文件计数器
     * @return 完整的文件路径
     */
    std::string generate_log_filename(const std::string& date, uint32_t counter);

    /**
     * @brief 获取当前日期字符串
     * @return YYYYMMDD格式的日期字符串
     */
    std::string get_current_date_string();

    /**
     * @brief 获取当前时间戳
     * @return ISO 8601格式的时间戳
     */
    std::string get_current_timestamp();

    /**
     * @brief 检查是否需要创建新文件
     * @return 是否需要创建新文件
     */
    bool should_create_new_file();

    /**
     * @brief 创建新的日志文件
     * @return 创建是否成功
     */
    bool create_new_log_file();

    /**
     * @brief 格式化坐标数据为日志行
     * @param coord_data 坐标数据
     * @return 格式化的日志行
     */
    std::string format_log_line(const CoordinateData& coord_data);

    /**
     * @brief WGS84坐标转换为GCJ02坐标
     * @param wgs_lon WGS84经度
     * @param wgs_lat WGS84纬度
     * @param gcj_lon GCJ02经度输出
     * @param gcj_lat GCJ02纬度输出
     */
    void wgs84_to_gcj02(double wgs_lon, double wgs_lat, double& gcj_lon, double& gcj_lat);

    /**
     * @brief 坐标转换辅助函数
     */
    double transform_lat(double x, double y);
    double transform_lon(double x, double y);

    /**
     * @brief 确保日志目录存在
     * @return 目录创建是否成功
     */
    bool ensure_log_directory();

    /**
     * @brief 获取下一个可用的文件计数器
     * @param date 日期字符串
     * @return 下一个可用的计数器值
     */
    uint32_t get_next_file_counter(const std::string& date);

    /**
     * @brief 添加数据到写入缓冲区
     * @param data 要添加的数据
     * @return 是否成功添加
     */
    bool add_to_buffer(const std::string& data);

    /**
     * @brief 获取当前时间戳 (毫秒)
     * @return 时间戳
     */
    uint64_t get_current_timestamp_ms();
};

/**
 * @brief GPS日志记录器工厂函数
 * @param sd_config SD卡配置
 * @param log_config 日志配置
 * @return GPS日志记录器实例
 */
std::unique_ptr<GPSLogger> create_gps_logger(
    const MicroSD::SPIConfig& sd_config = MicroSD::Config::DEFAULT,
    const GPSLogger::LogConfig& log_config = GPSLogger::LogConfig{}
);

} // namespace GPS
