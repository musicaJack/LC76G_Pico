/**
 * @file gps_logger.cpp
 * @brief GPS坐标日志记录器实现
 * @version 1.0.0
 */

#include "gps/gps_logger.hpp"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace GPS {

// =============================================================================
// 构造函数和析构函数
// =============================================================================

GPSLogger::GPSLogger(const SimpleSD::SPIConfig& sd_config, const LogConfig& log_config)
    : sd_card_(std::make_unique<SimpleSD::SimpleSDWriter>(sd_config))
    , config_(log_config)
    , current_file_size_(0)
    , daily_file_counter_(0)
    , is_initialized_(false)
    , log_file_created_(false)
    , buffer_used_(0)
    , pending_records_(0)
    , last_write_time_(0)
    , gaode_coordinate_count_(0) {
    
    // 初始化写入缓冲区
    write_buffer_.fill(0);
}

GPSLogger::~GPSLogger() {
    if (is_initialized_) {
        flush_buffer();  // 先刷新缓冲区
        sync();          // 再同步到SD卡
    }
}

GPSLogger::GPSLogger(GPSLogger&& other) noexcept
    : sd_card_(std::move(other.sd_card_))
    , config_(other.config_)
    , current_log_file_(std::move(other.current_log_file_))
    , current_file_size_(other.current_file_size_)
    , daily_file_counter_(other.daily_file_counter_)
    , current_date_(std::move(other.current_date_))
    , is_initialized_(other.is_initialized_) {
    other.is_initialized_ = false;
}

GPSLogger& GPSLogger::operator=(GPSLogger&& other) noexcept {
    if (this != &other) {
        if (is_initialized_) {
            sync();
        }
        
        sd_card_ = std::move(other.sd_card_);
        config_ = other.config_;
        current_log_file_ = std::move(other.current_log_file_);
        current_file_size_ = other.current_file_size_;
        daily_file_counter_ = other.daily_file_counter_;
        current_date_ = std::move(other.current_date_);
        is_initialized_ = other.is_initialized_;
        
        other.is_initialized_ = false;
    }
    return *this;
}

// =============================================================================
// 初始化方法
// =============================================================================

bool GPSLogger::initialize() {
    if (is_initialized_) {
        return true;
    }

    // 初始化SD卡
    if (!sd_card_->initialize()) {
        printf("[GPS Logger] SD卡初始化失败\n");
        return false;
    }

    // 确保日志目录存在
    if (!ensure_log_directory()) {
        printf("[GPS Logger] 创建日志目录失败\n");
        return false;
    }

    // 获取当前日期
    current_date_ = get_current_date_string();
    
    // 获取下一个可用的文件计数器
    daily_file_counter_ = get_next_file_counter(current_date_);
    
    // 创建第一个日志文件
    if (!create_new_log_file()) {
        printf("[GPS Logger] 创建日志文件失败\n");
        return false;
    }

    is_initialized_ = true;
    printf("[GPS Logger] 初始化成功，当前日志文件: %s\n", current_log_file_.c_str());
    return true;
}

// =============================================================================
// 日志记录方法
// =============================================================================

bool GPSLogger::log_gps_data(const LC76G_GPS_Data& gps_data) {
    if (!is_initialized_) {
        printf("[GPS Logger] 未初始化\n");
        return false;
    }

    // 检查GPS数据有效性
    if (!gps_data.Status) {
        printf("[GPS Logger] GPS数据无效，跳过记录\n");
        return false;
    }

    // 如果日志文件尚未创建，尝试基于GPS日期创建
    if (!log_file_created_) {
        if (!create_log_file_from_gps_date(gps_data)) {
            printf("[GPS Logger] 无法创建日志文件，跳过记录\n");
            return false;
        }
    }

    // 创建坐标数据
    CoordinateData coord_data = create_coordinate_data(gps_data);
    
    // 记录坐标数据
    return log_coordinate_data(coord_data);
}

bool GPSLogger::log_coordinate_data(const CoordinateData& coord_data) {
    if (!is_initialized_) {
        printf("[GPS Logger] 未初始化\n");
        return false;
    }

    // 检查数据有效性
    if (!coord_data.is_valid) {
        printf("[GPS Logger] 坐标数据无效，跳过记录\n");
        return false;
    }

    // 检查是否需要创建新文件
    if (should_create_new_file()) {
        if (!create_new_log_file()) {
            printf("[GPS Logger] 创建新日志文件失败\n");
            return false;
        }
    }

    // 更新高德API格式坐标数组
    if (gaode_coordinate_count_ < MAX_COORDINATES) {
        gaode_coordinates_[gaode_coordinate_count_] = {coord_data.longitude_gcj02, coord_data.latitude_gcj02};
        gaode_coordinate_count_++;
    } else {
        // 数组已满，可以选择覆盖最旧的记录或跳过
        // 这里选择覆盖最旧的记录（循环缓冲区）
        static size_t write_index = 0;
        gaode_coordinates_[write_index] = {coord_data.longitude_gcj02, coord_data.latitude_gcj02};
        write_index = (write_index + 1) % MAX_COORDINATES;
    }
    
    // 格式化日志行
    std::string log_line = format_log_line(coord_data);
    
    // 内存优化的批量写入策略
    if (config_.enable_immediate_write) {
        // 立即写入模式 (调试用)
        if (!sd_card_->append_text_file(current_log_file_, log_line)) {
            printf("[GPS Logger] 写入日志失败\n");
            return false;
        }
        current_file_size_ += log_line.length();
    } else {
        // 批量写入模式 (生产用)
        if (!add_to_buffer(log_line)) {
            // 缓冲区满，先写入再添加
            if (!flush_buffer()) {
                printf("[GPS Logger] 刷新缓冲区失败\n");
                return false;
            }
            if (!add_to_buffer(log_line)) {
                printf("[GPS Logger] 添加数据到缓冲区失败\n");
                return false;
            }
        }
        
        // 检查是否需要批量写入
        if (should_batch_write()) {
            if (!flush_buffer()) {
                printf("[GPS Logger] 批量写入失败\n");
                return false;
            }
        }
    }
    
    printf("[GPS Logger] 记录坐标: %.6f,%.6f (GCJ02: %.6f,%.6f) [缓冲:%zu]\n", 
           coord_data.longitude, coord_data.latitude,
           coord_data.longitude_gcj02, coord_data.latitude_gcj02,
           pending_records_);
    
    return true;
}

GPSLogger::CoordinateData GPSLogger::create_coordinate_data(const LC76G_GPS_Data& gps_data) {
    CoordinateData coord_data;
    
    // 基本坐标信息
    coord_data.longitude = gps_data.Lon;
    coord_data.latitude = gps_data.Lat;
    coord_data.altitude = gps_data.Altitude;      // 海拔高度
    coord_data.course = gps_data.Course;          // 航向
    coord_data.satellites = gps_data.Satellites;
    coord_data.hdop = gps_data.HDOP;
    coord_data.is_valid = gps_data.Status != 0;
    
    // 生成时间戳
    coord_data.timestamp = get_current_timestamp();
    
    // 坐标转换 (WGS84 -> GCJ02)
    if (config_.enable_coordinate_transform) {
        wgs84_to_gcj02(coord_data.longitude, coord_data.latitude, 
                       coord_data.longitude_gcj02, coord_data.latitude_gcj02);
    } else {
        coord_data.longitude_gcj02 = coord_data.longitude;
        coord_data.latitude_gcj02 = coord_data.latitude;
    }
    
    return coord_data;
}

// =============================================================================
// 文件管理方法
// =============================================================================

std::vector<std::string> GPSLogger::get_log_files() const {
    std::vector<std::string> log_files;
    
    if (!is_initialized_) {
        return log_files;
    }

    // 简化实现：返回空列表（实际项目中可以实现目录扫描）
    // 暂时跳过文件列表功能

    // 按文件名排序
    std::sort(log_files.begin(), log_files.end());
    return log_files;
}

size_t GPSLogger::cleanup_old_logs(int days_to_keep) {
    if (!is_initialized_) {
        return 0;
    }

    size_t deleted_count = 0;
    auto log_files = get_log_files();
    
    // 获取当前时间
    time_t current_time = time(nullptr);
    time_t cutoff_time = current_time - (days_to_keep * 24 * 60 * 60);
    
    for (const auto& file_path : log_files) {
        // 简化实现：跳过基于时间的文件清理
        // 实际项目中可以实现文件时间检查
        printf("[GPS Logger] 文件: %s\n", file_path.c_str());
    }
    
    return deleted_count;
}

std::string GPSLogger::get_log_statistics() const {
    std::ostringstream oss;
    
    if (!is_initialized_) {
        oss << "GPS Logger: 未初始化\n";
        return oss.str();
    }
    
    auto log_files = get_log_files();
    oss << "=== GPS日志统计 ===\n";
    oss << "日志目录: " << config_.log_directory << "\n";
    oss << "当前日志文件: " << current_log_file_ << "\n";
    oss << "当前文件大小: " << current_file_size_ << " 字节\n";
    oss << "日志文件总数: " << log_files.size() << "\n";
    oss << "坐标转换: " << (config_.enable_coordinate_transform ? "启用" : "禁用") << "\n";
    oss << "SD卡状态: 已连接\n";
    
    return oss.str();
}

std::string GPSLogger::generate_gaode_api_format() const {
    std::ostringstream oss;
    
    if (!is_initialized_ || gaode_coordinate_count_ == 0) {
        return "[]";
    }
    
    oss << "[";
    for (size_t i = 0; i < gaode_coordinate_count_; ++i) {
        if (i > 0) oss << ",";
        oss << "[" << std::fixed << std::setprecision(6) 
            << gaode_coordinates_[i].first << "," 
            << gaode_coordinates_[i].second << "]";
    }
    oss << "]";
    
    return oss.str();
}

bool GPSLogger::write_gaode_api_format() {
    if (!is_initialized_ || gaode_coordinate_count_ == 0) {
        return false;
    }
    
    // 生成高德API格式的文件名
    std::string gaode_filename = current_log_file_;
    size_t last_dot = gaode_filename.find_last_of('.');
    if (last_dot != std::string::npos) {
        gaode_filename = gaode_filename.substr(0, last_dot) + "_gaode.js";
    } else {
        gaode_filename += "_gaode.js";
    }
    
    // 生成JavaScript格式的坐标数组
    std::ostringstream js_content;
    js_content << "// 高德地图轨迹回放坐标数据\n";
    js_content << "// 生成时间: " << get_current_timestamp() << "\n";
    js_content << "// 坐标系: GCJ02 (高德地图)\n";
    js_content << "var lineArr = " << generate_gaode_api_format() << ";\n";
    js_content << "\n";
    js_content << "// 使用示例:\n";
    js_content << "// marker.moveAlong(lineArr, {\n";
    js_content << "//     duration: 500,\n";
    js_content << "//     autoRotation: true,\n";
    js_content << "// });\n";
    
    // 写入文件
    if (!sd_card_->write_text_file(gaode_filename, js_content.str())) {
        printf("[GPS Logger] 写入高德API格式文件失败: %s\n", gaode_filename.c_str());
        return false;
    }
    
    printf("[GPS Logger] 高德API格式文件已生成: %s\n", gaode_filename.c_str());
    return true;
}

bool GPSLogger::sync() {
    if (!is_initialized_) {
        return false;
    }
    
    // 先刷新缓冲区
    if (!flush_buffer()) {
        return false;
    }
    
    // 简化实现：SD卡同步总是成功
    return true;
}

bool GPSLogger::flush_buffer() {
    if (!is_initialized_ || buffer_used_ == 0) {
        return true;
    }
    
    // 将缓冲区数据写入SD卡
    std::string buffer_data(write_buffer_.data(), buffer_used_);
    if (!sd_card_->append_text_file(current_log_file_, buffer_data)) {
        printf("[GPS Logger] 刷新缓冲区失败\n");
        return false;
    }
    
    // 更新文件大小和统计
    current_file_size_ += buffer_used_;
    printf("[GPS Logger] 批量写入 %zu 条记录 (%zu 字节)\n", pending_records_, buffer_used_);
    
    // 清空缓冲区
    buffer_used_ = 0;
    pending_records_ = 0;
    last_write_time_ = get_current_timestamp_ms();
    write_buffer_.fill(0);
    
    return true;
}

bool GPSLogger::should_batch_write() {
    // 检查记录数量
    if (pending_records_ >= config_.batch_write_count) {
        return true;
    }
    
    // 检查时间间隔
    uint64_t current_time = get_current_timestamp_ms();
    if (current_time - last_write_time_ >= config_.write_interval_ms) {
        return true;
    }
    
    // 检查缓冲区使用率
    if (buffer_used_ >= config_.buffer_size * 0.8) {  // 80%使用率时写入
        return true;
    }
    
    return false;
}

std::string GPSLogger::get_memory_usage() const {
    std::ostringstream oss;
    
    oss << "=== GPS Logger 内存使用 ===\n";
    oss << "缓冲区大小: " << write_buffer_.size() << " 字节\n";
    oss << "缓冲区使用: " << buffer_used_ << " 字节 (" 
        << (buffer_used_ * 100 / write_buffer_.size()) << "%)\n";
    oss << "待写入记录: " << pending_records_ << " 条\n";
    oss << "配置缓冲区: " << config_.buffer_size << " 字节\n";
    oss << "批量写入数: " << config_.batch_write_count << " 条\n";
    oss << "写入间隔: " << config_.write_interval_ms << " 毫秒\n";
    
    return oss.str();
}

// =============================================================================
// 私有辅助方法
// =============================================================================

std::string GPSLogger::generate_log_filename(const std::string& date, uint32_t counter) {
    std::ostringstream oss;
    oss << config_.log_directory << "/" << date << "_" 
        << std::setfill('0') << std::setw(3) << counter << config_.file_extension;
    return oss.str();
}

std::string GPSLogger::get_current_date_string() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    std::ostringstream oss;
    oss << std::setfill('0') 
        << std::setw(4) << (timeinfo->tm_year + 1900)
        << std::setw(2) << (timeinfo->tm_mon + 1)
        << std::setw(2) << timeinfo->tm_mday;
    
    return oss.str();
}

std::string GPSLogger::get_current_timestamp() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (timeinfo->tm_year + 1900) << "-"
        << std::setw(2) << (timeinfo->tm_mon + 1) << "-"
        << std::setw(2) << timeinfo->tm_mday << "T"
        << std::setw(2) << timeinfo->tm_hour << ":"
        << std::setw(2) << timeinfo->tm_min << ":"
        << std::setw(2) << timeinfo->tm_sec << "Z";
    
    return oss.str();
}

bool GPSLogger::should_create_new_file() {
    // 检查日期是否变化
    std::string today = get_current_date_string();
    if (today != current_date_) {
        current_date_ = today;
        daily_file_counter_ = 0;
        return true;
    }
    
    // 检查文件大小是否超限
    if (current_file_size_ >= config_.max_file_size) {
        return true;
    }
    
    // 检查每日文件数量是否超限
    if (daily_file_counter_ >= config_.max_files_per_day) {
        return true;
    }
    
    return false;
}

bool GPSLogger::create_new_log_file() {
    // 获取下一个可用的文件计数器
    daily_file_counter_ = get_next_file_counter(current_date_);
    
    // 生成新的文件名
    current_log_file_ = generate_log_filename(current_date_, daily_file_counter_);
    
    // 创建文件并写入头部信息
    std::ostringstream header;
    header << "# GPS轨迹日志文件\n";
    header << "# 创建时间: " << get_current_timestamp() << "\n";
    header << "# 格式: 时间戳,经度,纬度,GCJ02经度,GCJ02纬度,高度,航向,卫星数,HDOP,有效性\n";
    header << "# 坐标系: WGS84 -> GCJ02 (高德地图)\n";
    header << "# 高德API格式: [[经度,纬度],[经度,纬度],...]\n";
    header << "# 注意: 高德API使用GCJ02坐标系\n";
    
    if (!sd_card_->write_text_file(current_log_file_, header.str())) {
        printf("[GPS Logger] 创建日志文件失败\n");
        return false;
    }
    
    current_file_size_ = header.str().length();
    printf("[GPS Logger] 创建新日志文件: %s\n", current_log_file_.c_str());
    
    return true;
}

std::string GPSLogger::format_log_line(const CoordinateData& coord_data) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6)
        << coord_data.timestamp << ","
        << coord_data.longitude << ","
        << coord_data.latitude << ","
        << coord_data.longitude_gcj02 << ","
        << coord_data.latitude_gcj02 << ","
        << std::setprecision(1) << coord_data.altitude << ","
        << std::setprecision(1) << coord_data.course << ","
        << (int)coord_data.satellites << ","
        << std::setprecision(2) << coord_data.hdop << ","
        << (coord_data.is_valid ? "true" : "false") << "\n";

    return oss.str();
}

void GPSLogger::wgs84_to_gcj02(double wgs_lon, double wgs_lat, double& gcj_lon, double& gcj_lat) {
    double dLat = transform_lat(wgs_lon - 105.0, wgs_lat - 35.0);
    double dLon = transform_lon(wgs_lon - 105.0, wgs_lat - 35.0);
    double radLat = wgs_lat / 180.0 * PI;
    double magic = sin(radLat);
    magic = 1 - EE * magic * magic;
    double sqrtMagic = sqrt(magic);
    dLat = (dLat * 180.0) / ((A * (1 - EE)) / (magic * sqrtMagic) * PI);
    dLon = (dLon * 180.0) / (A / sqrtMagic * cos(radLat) * PI);
    gcj_lat = wgs_lat + dLat;
    gcj_lon = wgs_lon + dLon;
}

double GPSLogger::transform_lat(double x, double y) {
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * PI) + 20.0 * sin(2.0 * x * PI)) * 2.0 / 3.0;
    ret += (20.0 * sin(y * PI) + 40.0 * sin(y / 3.0 * PI)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * PI) + 320 * sin(y * PI / 30.0)) * 2.0 / 3.0;
    return ret;
}

double GPSLogger::transform_lon(double x, double y) {
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * PI) + 20.0 * sin(2.0 * x * PI)) * 2.0 / 3.0;
    ret += (20.0 * sin(x * PI) + 40.0 * sin(x / 3.0 * PI)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * PI) + 300.0 * sin(x / 30.0 * PI)) * 2.0 / 3.0;
    return ret;
}

bool GPSLogger::ensure_log_directory() {
    if (!config_.auto_create_directory) {
        return true;
    }
    
    if (!sd_card_->create_directory(config_.log_directory)) {
        printf("[GPS Logger] 创建日志目录失败\n");
        return false;
    }
    
    return true;
}

uint32_t GPSLogger::get_next_file_counter(const std::string& date) {
    auto log_files = get_log_files();
    uint32_t max_counter = 0;
    
    std::string prefix = date + "_";
    for (const auto& file_path : log_files) {
        std::string filename = file_path.substr(file_path.find_last_of("/") + 1);
        if (filename.substr(0, prefix.length()) == prefix) {
            // 提取计数器部分
            std::string counter_str = filename.substr(prefix.length(), 3);
            // 简单的字符串转数字，不使用异常处理
            uint32_t counter = 0;
            bool valid = true;
            for (char c : counter_str) {
                if (c >= '0' && c <= '9') {
                    counter = counter * 10 + (c - '0');
                } else {
                    valid = false;
                    break;
                }
            }
            if (valid && counter > max_counter) {
                max_counter = counter;
            }
        }
    }
    
    return max_counter + 1;
}

bool GPSLogger::add_to_buffer(const std::string& data) {
    size_t data_len = data.length();
    
    // 检查缓冲区是否有足够空间
    if (buffer_used_ + data_len >= write_buffer_.size()) {
        return false;  // 缓冲区空间不足
    }
    
    // 将数据复制到缓冲区
    memcpy(write_buffer_.data() + buffer_used_, data.c_str(), data_len);
    buffer_used_ += data_len;
    pending_records_++;
    
    return true;
}

uint64_t GPSLogger::get_current_timestamp_ms() {
    return to_ms_since_boot(get_absolute_time());
}

std::string GPSLogger::extract_datetime_from_gps(const LC76G_GPS_Data& gps_data) {
    // 从GPS数据中提取日期和时间，格式：YYYY-MM-DD_HH:MM:SS
    std::string gps_date = gps_data.Date;
    
    if (gps_date.length() >= 10 && gps_date[4] == '-' && gps_date[7] == '-') {
        // 格式：YYYY-MM-DD，添加时间部分
        char datetime[32];
        snprintf(datetime, sizeof(datetime), "%s_%02d:%02d:%02d", 
                 gps_date.c_str(), gps_data.Time_H, gps_data.Time_M, gps_data.Time_S);
        return std::string(datetime);
    }
    
    return "";  // 日期格式无效
}

bool GPSLogger::create_log_file_from_gps_date(const LC76G_GPS_Data& gps_data) {
    if (log_file_created_) {
        return true;  // 文件已创建
    }
    
    // 从GPS数据中提取日期时间
    std::string gps_datetime = extract_datetime_from_gps(gps_data);
    if (gps_datetime.empty()) {
        printf("[GPS Logger] 无法从GPS数据中提取有效日期时间\n");
        return false;
    }
    
    // 生成文件名：YYYY-MM-DD_HH:MM:SS.log
    std::string filename = gps_datetime + config_.file_extension;
    current_log_file_ = config_.log_directory + "/" + filename;
    current_file_size_ = 0;
    
    // 创建日志文件
    if (!create_new_log_file()) {
        printf("[GPS Logger] 创建日志文件失败: %s\n", current_log_file_.c_str());
        return false;
    }
    
    log_file_created_ = true;
    printf("[GPS Logger] 基于GPS日期时间创建日志文件: %s\n", current_log_file_.c_str());
    return true;
}

// =============================================================================
// 工厂函数
// =============================================================================

std::unique_ptr<GPSLogger> create_gps_logger(
    const SimpleSD::SPIConfig& sd_config,
    const GPSLogger::LogConfig& log_config) {
    
    auto logger = std::make_unique<GPSLogger>(sd_config, log_config);
    if (!logger->initialize()) {
        return nullptr;
    }
    
    return logger;
}

} // namespace GPS
