/**
 * @file simple_sd_writer.hpp
 * @brief 简化的SD卡写入器 - 基于pico_fatfs
 * @version 1.0.0
 * 
 * 功能特性:
 * - 基于pico_fatfs的简化SD卡操作
 * - 支持文件创建、写入、追加
 * - 适配RP2040内存限制
 * - 错误处理和状态管理
 */

#pragma once

#include <string>
#include <memory>
#include <cstdint>

namespace SimpleSD {

/**
 * @brief SD卡配置结构
 */
struct SPIConfig {
    uint8_t spi_instance = 0;    // SPI实例 (0或1)
    uint8_t cs_pin = 17;         // CS引脚
    uint8_t mosi_pin = 19;       // MOSI引脚
    uint8_t miso_pin = 16;       // MISO引脚
    uint8_t sck_pin = 18;        // SCK引脚
    uint32_t baudrate = 1000000; // 波特率
};

/**
 * @brief 简化的SD卡写入器类
 */
class SimpleSDWriter {
public:
    /**
     * @brief 构造函数
     * @param config SPI配置
     */
    explicit SimpleSDWriter(const SPIConfig& config);
    
    /**
     * @brief 析构函数
     */
    ~SimpleSDWriter();
    
    /**
     * @brief 初始化SD卡
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * @brief 检查SD卡是否已初始化
     * @return 是否已初始化
     */
    bool is_initialized() const;
    
    /**
     * @brief 创建目录
     * @param path 目录路径
     * @return 创建是否成功
     */
    bool create_directory(const std::string& path);
    
    /**
     * @brief 追加文本到文件
     * @param filepath 文件路径
     * @param content 内容
     * @return 写入是否成功
     */
    bool append_text_file(const std::string& filepath, const std::string& content);
    
    /**
     * @brief 写入文本到文件
     * @param filepath 文件路径
     * @param content 内容
     * @return 写入是否成功
     */
    bool write_text_file(const std::string& filepath, const std::string& content);
    
    /**
     * @brief 检查文件是否存在
     * @param filepath 文件路径
     * @return 文件是否存在
     */
    bool file_exists(const std::string& filepath);
    
    /**
     * @brief 获取文件大小
     * @param filepath 文件路径
     * @return 文件大小 (字节)
     */
    uint32_t get_file_size(const std::string& filepath);

private:
    SPIConfig config_;
    bool initialized_;
    
    /**
     * @brief 初始化SPI
     * @return 初始化是否成功
     */
    bool init_spi();
    
    /**
     * @brief 初始化FatFS
     * @return 初始化是否成功
     */
    bool init_fatfs();
};

} // namespace SimpleSD
