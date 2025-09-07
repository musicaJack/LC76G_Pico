#pragma once

/**
 * @file pin_config.hpp
 * @brief 统一的引脚配置管理 - ILI9488项目
 * @description 这个文件包含了所有硬件引脚的定义，包括SPI屏幕和joystick的引脚配置
 */

// 前向声明，避免头文件循环依赖
typedef struct spi_inst spi_inst_t;
typedef struct i2c_inst i2c_inst_t;

// =============================================================================
// ILI9488 SPI 显示屏引脚配置
// =============================================================================

// SPI 接口配置
#define ILI9488_SPI_INST        spi0        // SPI接口实例
#define ILI9488_SPI_SPEED_HZ    40000000    // SPI速度（40MHz）

// SPI 信号引脚
#define ILI9488_PIN_SCK         18          // SPI时钟引脚
#define ILI9488_PIN_MOSI        19          // SPI数据输出引脚

// 控制信号引脚
#define ILI9488_PIN_CS          17          // 片选引脚
#define ILI9488_PIN_DC          20          // 数据/命令选择引脚
#define ILI9488_PIN_RST         15          // 复位引脚
#define ILI9488_PIN_BL          16          // 背光控制引脚

// =============================================================================
// GPS模块 UART 配置
// =============================================================================

// UART 接口配置
#define GPS_UART_ID             0           // UART接口ID (UART0)
#define GPS_BAUD_RATE           115200      // GPS模块波特率
#define GPS_TX_PIN              0           // UART TX引脚
#define GPS_RX_PIN              1           // UART RX引脚
#define GPS_FORCE_PIN           4           // FORCE引脚（GPS模块控制）

// GPS 操作参数
#define GPS_UPDATE_INTERVAL_MS  1000        // GPS数据更新间隔（毫秒）
#define GPS_TIMEOUT_MS          5000        // GPS超时时间（毫秒）

// =============================================================================
// 按键控制配置
// =============================================================================

// 按键引脚配置
#define BUTTON_PIN                 14          // 按键引脚（GPIO-14）
#define BUTTON_SHORT_PRESS_MS      1000        // 短按时间阈值（毫秒）
#define BUTTON_LONG_PRESS_MS       1000        // 长按时间阈值（毫秒）
#define BUTTON_DEBOUNCE_MS         50          // 按键防抖时间（毫秒）

// =============================================================================
// Joystick 手柄 I2C 配置
// =============================================================================

// I2C 接口配置
#define JOYSTICK_I2C_INST       i2c1        // I2C接口实例
#define JOYSTICK_I2C_ADDR       0x63        // I2C设备地址
#define JOYSTICK_I2C_SPEED      100000      // I2C速度（100kHz）

// I2C 信号引脚
#define JOYSTICK_PIN_SDA        6           // I2C数据引脚
#define JOYSTICK_PIN_SCL        7           // I2C时钟引脚

// Joystick 操作参数配置
#define JOYSTICK_THRESHOLD      1800        // 操作检测阈值
#define JOYSTICK_LOOP_DELAY_MS  20          // 循环延迟时间（毫秒）

// Joystick LED 颜色定义
#define JOYSTICK_LED_OFF        0x000000    // 黑色（关闭）
#define JOYSTICK_LED_RED        0xFF0000    // 红色
#define JOYSTICK_LED_GREEN      0x00FF00    // 绿色
#define JOYSTICK_LED_BLUE       0x0000FF    // 蓝色

// =============================================================================
// 兼容性宏定义（保持向后兼容）
// =============================================================================

// ILI9488 兼容性定义
#define PIN_DC                  ILI9488_PIN_DC
#define PIN_RST                 ILI9488_PIN_RST
#define PIN_CS                  ILI9488_PIN_CS
#define PIN_SCK                 ILI9488_PIN_SCK
#define PIN_MOSI                ILI9488_PIN_MOSI
#define PIN_BL                  ILI9488_PIN_BL

// Joystick 兼容性定义（保持向后兼容）
#define JOYSTICK_I2C_PORT       JOYSTICK_I2C_INST
#define JOYSTICK_I2C_SDA_PIN    JOYSTICK_PIN_SDA
#define JOYSTICK_I2C_SCL_PIN    JOYSTICK_PIN_SCL

// =============================================================================
// 硬件配置验证宏
// =============================================================================

// 编译时验证引脚配置的合理性
#if ILI9488_PIN_SCK == JOYSTICK_PIN_SDA || ILI9488_PIN_SCK == JOYSTICK_PIN_SCL
    #warning "SPI SCK pin conflicts with I2C pins"
#endif

#if ILI9488_PIN_MOSI == JOYSTICK_PIN_SDA || ILI9488_PIN_MOSI == JOYSTICK_PIN_SCL
    #warning "SPI MOSI pin conflicts with I2C pins"
#endif

// =============================================================================
// 辅助宏定义
// =============================================================================

// 获取完整的SPI配置
#define ILI9488_GET_SPI_CONFIG() ILI9488_SPI_INST, ILI9488_PIN_DC, ILI9488_PIN_RST, ILI9488_PIN_CS, ILI9488_PIN_SCK, ILI9488_PIN_MOSI, ILI9488_PIN_BL, ILI9488_SPI_SPEED_HZ

// 获取完整的Joystick配置
#define JOYSTICK_GET_I2C_CONFIG() JOYSTICK_I2C_INST, JOYSTICK_I2C_ADDR, JOYSTICK_PIN_SDA, JOYSTICK_PIN_SCL, JOYSTICK_I2C_SPEED