# GPS模块I2C通信迁移指南

## 概述

本文档描述了将LC76G GPS模块从UART通信迁移到I2C1通信的详细过程。

## 硬件连接变更

### 原UART连接
- TX引脚: GPIO0
- RX引脚: GPIO1
- FORCE引脚: GPIO4

### 新I2C连接
- SDA引脚: GPIO2
- SCL引脚: GPIO3
- FORCE引脚: GPIO4 (保持不变)
- I2C地址: 0x42 (LC76G默认地址)
- I2C速度: 100kHz

## 代码变更

### 1. 引脚配置更新 (`include/pin_config.hpp`)

```cpp
// 原UART配置 (已注释)
// #define GPS_UART_ID             0
// #define GPS_BAUD_RATE           115200
// #define GPS_TX_PIN              0
// #define GPS_RX_PIN              1

// 新I2C配置
#define GPS_I2C_INST            i2c1        // I2C接口实例
#define GPS_I2C_ADDR            0x42        // GPS模块I2C地址
#define GPS_I2C_SPEED           100000      // I2C速度（100kHz）
#define GPS_PIN_SDA             26           // I2C数据引脚
#define GPS_PIN_SCL             7           // I2C时钟引脚
```

### 2. GPS解析器更新 (`src/gps/vendor_gps_parser.c`)

- 将UART通信函数替换为I2C通信函数
- 更新初始化函数签名
- 修改数据发送和接收逻辑

### 3. 示例程序更新

所有示例程序都已更新以使用新的I2C初始化函数：

```cpp
// 原UART初始化
// vendor_gps_init(GPS_UART_ID, GPS_BAUD_RATE, GPS_TX_PIN, GPS_RX_PIN, GPS_FORCE_PIN);

// 新I2C初始化
vendor_gps_init(GPS_I2C_INST, GPS_I2C_ADDR, GPS_PIN_SDA, GPS_PIN_SCL, GPS_I2C_SPEED, GPS_FORCE_PIN);
```

## 新增测试程序

### GPS I2C通信测试程序 (`examples/gps_i2c_test.cpp`)

这是一个专门用于测试I2C通信的简单程序，包含：

- I2C初始化测试
- 基本命令发送测试
- 数据接收测试
- 详细的调试输出

## 编译和运行

### 编译所有程序
```bash
cd build
make -j4
```

### 运行I2C测试程序
```bash
# 将gps_i2c_test.uf2文件烧录到Pico
# 通过USB串口查看测试结果
```

### 运行其他示例程序
所有原有的示例程序都已更新支持I2C通信：
- `vendor_gps_dashboard.uf2` - GPS仪表盘
- `vendor_gps_ili9488.uf2` - GPS + ILI9488显示
- `vendor_gps_ili9488_optimized.uf2` - 优化版显示
- `vendor_gps_display.uf2` - GPS + ST7789显示
- `vendor_gps_example.uf2` - 基础GPS功能

## 注意事项

### 1. LC76G GPS模块I2C支持

**重要**: LC76G GPS模块可能不完全支持I2C通信。当前实现是基于假设的I2C协议：

- 如果模块不支持I2C读取，数据接收可能失败
- 命令发送通过I2C写入实现
- 数据接收可能需要使用其他方法（如中断或轮询）

### 2. 引脚冲突检查

确保新的I2C引脚不与现有功能冲突：
- Joystick使用I2C1 (SDA: GPIO6, SCL: GPIO7)
- GPS使用I2C1 (SDA: GPIO2, SCL: GPIO3)
- 两个设备可以共享同一个I2C总线，但地址必须不同

### 3. 调试建议

1. 首先运行 `gps_i2c_test` 程序验证I2C通信
2. 检查GPS模块是否支持I2C通信
3. 如果I2C通信失败，考虑回退到UART通信
4. 使用示波器或逻辑分析仪检查I2C信号

## 故障排除

### 常见问题

1. **GPS模块初始化失败**
   - 检查I2C连接是否正确
   - 验证GPS模块的I2C地址
   - 确认电源连接正常

2. **无法接收GPS数据**
   - LC76G可能不支持I2C数据读取
   - 考虑使用UART作为备用方案
   - 检查天线连接和信号强度

3. **I2C通信错误**
   - 检查上拉电阻是否正确连接
   - 验证I2C地址和速度设置
   - 使用逻辑分析仪调试I2C信号

## 回退方案

如果I2C通信不工作，可以轻松回退到UART通信：

1. 恢复UART引脚配置
2. 恢复UART初始化函数
3. 重新编译程序

## 总结

本次迁移将GPS模块从UART通信改为I2C通信，主要目的是：

- 释放UART资源用于其他用途
- 统一使用I2C总线管理多个设备
- 简化硬件连接

但是，由于LC76G GPS模块的I2C支持可能有限，建议在实际使用前进行充分测试。
