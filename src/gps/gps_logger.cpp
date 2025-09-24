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

GPSLogger::GPSLogger(const MicroSD::SPIConfig& sd_config, const LogConfig& log_config)
    : sd_card_(std::make_unique<MicroSD::RWSD>(sd_config))
    , config_(log_config)
    , current_file_size_(0)
    , daily_file_counter_(0)
    , is_initialized_(false)
    , buffer_used_(0)
    , pending_records_(0)
    , last_write_time_(0) {
    
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
    auto init_result = sd_card_->initialize();
    if (!init_result.is_ok()) {
        printf("[GPS Logger] SD卡初始化失败: %s\n", init_result.error_message().c_str());
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

    // 格式化日志行
    std::string log_line = format_log_line(coord_data);
    
    // 内存优化的批量写入策略
    if (config_.enable_immediate_write) {
        // 立即写入模式 (调试用)
        auto result = sd_card_->append_text_file(current_log_file_, log_line);
        if (!result.is_ok()) {
            printf("[GPS Logger] 写入日志失败: %s\n", result.error_message().c_str());
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

    auto result = sd_card_->list_directory(config_.log_directory);
    if (!result.is_ok()) {
        printf("[GPS Logger] 获取日志文件列表失败: %s\n", result.error_message().c_str());
        return log_files;
    }

    auto files = *result;
    for (const auto& file : files) {
        if (!file.is_directory && file.name.find(config_.file_extension) != std::string::npos) {
            log_files.push_back(file.full_path);
        }
    }

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
        auto file_info = sd_card_->get_file_info(file_path);
        if (file_info.is_ok()) {
            // 这里需要根据文件修改时间判断，但FatFS可能不提供修改时间
            // 暂时跳过基于时间的清理，只提供基于文件数量的清理
            printf("[GPS Logger] 文件: %s\n", file_path.c_str());
        }
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
    auto capacity_result = sd_card_->get_capacity();
    
    oss << "=== GPS日志统计 ===\n";
    oss << "日志目录: " << config_.log_directory << "\n";
    oss << "当前日志文件: " << current_log_file_ << "\n";
    oss << "当前文件大小: " << current_file_size_ << " 字节\n";
    oss << "日志文件总数: " << log_files.size() << "\n";
    oss << "坐标转换: " << (config_.enable_coordinate_transform ? "启用" : "禁用") << "\n";
    
    if (capacity_result.is_ok()) {
        auto [total, free] = *capacity_result;
        oss << "SD卡总容量: " << (total / 1024 / 1024) << " MB\n";
        oss << "SD卡可用容量: " << (free / 1024 / 1024) << " MB\n";
    }
    
    return oss.str();
}

bool GPSLogger::sync() {
    if (!is_initialized_) {
        return false;
    }
    
    // 先刷新缓冲区
    if (!flush_buffer()) {
        return false;
    }
    
    auto result = sd_card_->sync();
    return result.is_ok();
}

bool GPSLogger::flush_buffer() {
    if (!is_initialized_ || buffer_used_ == 0) {
        return true;
    }
    
    // 将缓冲区数据写入SD卡
    std::string buffer_data(write_buffer_.data(), buffer_used_);
    auto result = sd_card_->append_text_file(current_log_file_, buffer_data);
    
    if (!result.is_ok()) {
        printf("[GPS Logger] 刷新缓冲区失败: %s\n", result.error_message().c_str());
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
    header << "# 格式: 经度,纬度,时间戳,卫星数,HDOP,经度(GCJ02),纬度(GCJ02)\n";
    header << "# 坐标系: WGS84 -> GCJ02 (高德地图)\n";
    
    auto result = sd_card_->write_text_file(current_log_file_, header.str());
    if (!result.is_ok()) {
        printf("[GPS Logger] 创建日志文件失败: %s\n", result.error_message().c_str());
        return false;
    }
    
    current_file_size_ = header.str().length();
    printf("[GPS Logger] 创建新日志文件: %s\n", current_log_file_.c_str());
    
    return true;
}

std::string GPSLogger::format_log_line(const CoordinateData& coord_data) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6)
        << coord_data.longitude << ","
        << coord_data.latitude << ","
        << coord_data.timestamp << ","
        << (int)coord_data.satellites << ","
        << std::setprecision(2) << coord_data.hdop << ","
        << std::setprecision(6) << coord_data.longitude_gcj02 << ","
        << coord_data.latitude_gcj02 << "\n";
    
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
    
    auto result = sd_card_->create_directory(config_.log_directory);
    if (!result.is_ok()) {
        // 如果目录已存在，这不是错误
        if (result.error_code() != MicroSD::ErrorCode::INVALID_PARAMETER) {
            printf("[GPS Logger] 创建日志目录失败: %s\n", result.error_message().c_str());
            return false;
        }
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

// =============================================================================
// 工厂函数
// =============================================================================

std::unique_ptr<GPSLogger> create_gps_logger(
    const MicroSD::SPIConfig& sd_config,
    const GPSLogger::LogConfig& log_config) {
    
    auto logger = std::make_unique<GPSLogger>(sd_config, log_config);
    if (!logger->initialize()) {
        return nullptr;
    }
    
    return logger;
}

} // namespace GPS
