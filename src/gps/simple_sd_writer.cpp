/**
 * @file simple_sd_writer.cpp
 * @brief 简化的SD卡写入器实现
 */

#include "gps/simple_sd_writer.hpp"
#include "tf_card.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <cstdio>
#include <cstring>

namespace SimpleSD {

SimpleSDWriter::SimpleSDWriter(const SPIConfig& config)
    : config_(config), initialized_(false) {
}

SimpleSDWriter::~SimpleSDWriter() {
    // 清理资源
    if (initialized_) {
        // 卸载文件系统
        f_unmount("0:");
    }
}

bool SimpleSDWriter::initialize() {
    if (initialized_) {
        return true;
    }
    
    // 初始化SPI
    if (!init_spi()) {
        printf("[SimpleSD] SPI初始化失败\n");
        return false;
    }
    
    // 初始化FatFS
    if (!init_fatfs()) {
        printf("[SimpleSD] FatFS初始化失败\n");
        return false;
    }
    
    initialized_ = true;
    printf("[SimpleSD] SD卡初始化成功\n");
    return true;
}

bool SimpleSDWriter::is_initialized() const {
    return initialized_;
}

bool SimpleSDWriter::create_directory(const std::string& path) {
    if (!initialized_) {
        return false;
    }
    
    FRESULT res = f_mkdir(path.c_str());
    if (res == FR_OK || res == FR_EXIST) {
        return true;
    }
    
    printf("[SimpleSD] 创建目录失败: %s (错误: %d)\n", path.c_str(), res);
    return false;
}

bool SimpleSDWriter::append_text_file(const std::string& filepath, const std::string& content) {
    if (!initialized_) {
        return false;
    }
    
    FIL file;
    FRESULT res = f_open(&file, filepath.c_str(), FA_OPEN_APPEND | FA_WRITE);
    if (res != FR_OK) {
        printf("[SimpleSD] 打开文件失败: %s (错误: %d)\n", filepath.c_str(), res);
        return false;
    }
    
    UINT bytes_written;
    res = f_write(&file, content.c_str(), content.length(), &bytes_written);
    f_close(&file);
    
    if (res != FR_OK) {
        printf("[SimpleSD] 写入文件失败: %s (错误: %d)\n", filepath.c_str(), res);
        return false;
    }
    
    return true;
}

bool SimpleSDWriter::write_text_file(const std::string& filepath, const std::string& content) {
    if (!initialized_) {
        return false;
    }
    
    FIL file;
    FRESULT res = f_open(&file, filepath.c_str(), FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        printf("[SimpleSD] 创建文件失败: %s (错误: %d)\n", filepath.c_str(), res);
        return false;
    }
    
    UINT bytes_written;
    res = f_write(&file, content.c_str(), content.length(), &bytes_written);
    f_close(&file);
    
    if (res != FR_OK) {
        printf("[SimpleSD] 写入文件失败: %s (错误: %d)\n", filepath.c_str(), res);
        return false;
    }
    
    return true;
}

bool SimpleSDWriter::file_exists(const std::string& filepath) {
    if (!initialized_) {
        return false;
    }
    
    FILINFO file_info;
    FRESULT res = f_stat(filepath.c_str(), &file_info);
    return res == FR_OK;
}

uint32_t SimpleSDWriter::get_file_size(const std::string& filepath) {
    if (!initialized_) {
        return 0;
    }
    
    FILINFO file_info;
    FRESULT res = f_stat(filepath.c_str(), &file_info);
    if (res != FR_OK) {
        return 0;
    }
    
    return file_info.fsize;
}

bool SimpleSDWriter::init_spi() {
    // 初始化SPI
    spi_init(config_.spi_instance == 0 ? spi0 : spi1, config_.baudrate);
    
    // 设置SPI引脚
    gpio_set_function(config_.mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(config_.miso_pin, GPIO_FUNC_SPI);
    gpio_set_function(config_.sck_pin, GPIO_FUNC_SPI);
    
    // 设置CS引脚
    gpio_init(config_.cs_pin);
    gpio_set_dir(config_.cs_pin, GPIO_OUT);
    gpio_put(config_.cs_pin, 1); // CS高电平
    
    return true;
}

bool SimpleSDWriter::init_fatfs() {
    // 挂载SD卡
    static FATFS fatfs;
    FRESULT res = f_mount(&fatfs, "0:", 1);
    if (res != FR_OK) {
        printf("[SimpleSD] 挂载SD卡失败 (错误: %d)\n", res);
        return false;
    }
    
    return true;
}

} // namespace SimpleSD
