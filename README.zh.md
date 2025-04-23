# LC76G_Pico GPS 跟踪器

![许可证](https://img.shields.io/badge/许可证-MIT-blue.svg)
![平台](https://img.shields.io/badge/平台-Raspberry%20Pi%20Pico-brightgreen.svg)
![版本](https://img.shields.io/badge/版本-1.0.0-orange.svg)

[English](README.md) | 中文

## 概述

LC76G_Pico 是一个基于 Raspberry Pi Pico 微控制器和 L76X GPS 模块的开源 GPS 跟踪项目。该项目提供了一个完整的解决方案，用于获取、处理和显示 GPS 数据，使其成为位置跟踪应用、导航系统和需要地理空间功能的物联网设备的理想选择。

## 特性

- **全面的 GPS 数据处理**：准确地从 NMEA 语句中提取和处理地理坐标、海拔高度、速度和航向信息。
- **多坐标系统支持**：内置 WGS-84、GCJ-02（谷歌地图）和 BD-09（百度地图）坐标系统的转换功能。
- **模块化架构**：硬件驱动和应用逻辑之间的清晰分离，增强了可扩展性。
- **低功耗运行**：支持 L76X GPS 模块的待机模式和电源管理功能。
- **灵活配置**：可配置的 UART、GPIO 引脚和 GPS 模块参数。
- **强大的 NMEA 解析器**：通过适当的验证和错误处理可靠地解析 GPGGA 和 GPRMC 语句。
- **模拟数据支持**：包含模拟功能，无需物理 GPS 硬件即可进行测试。

## 硬件要求

- Raspberry Pi Pico/Pico W
- L76X GPS 模块
- 可选：ST7789 显示屏（240×320）用于可视化
- UART 连接线
- 电源（USB 或电池）

## 接线图

```
┌─────────────────┐                  ┌─────────────────┐
│                 │                  │                 │
│  Raspberry Pi   │                  │    L76X GPS     │
│     Pico        │                  │     模块        │
│                 │                  │                 │
│  GPIO 0 (TX)    │─────────────────→│  RX             │
│  GPIO 1 (RX)    │←─────────────────│  TX             │
│  VCC            │─────────────────→│  VCC            │
│  GND            │←────────────────→│  GND            │
│                 │                  │                 │
└─────────────────┘                  └─────────────────┘
```

## 软件架构

项目结构划分为以下组件：

- **GPS 解析器模块**：用于 GPS 数据提取和处理的核心库
- **显示模块**：用于渲染 GPS 信息的可选 UI 功能
- **示例应用**：展示库使用方法的演示程序

## 开始使用

### 前提条件

- [Raspberry Pi Pico C/C++ SDK](https://github.com/raspberrypi/pico-sdk)
- [CMake](https://cmake.org/)（最低版本 3.13）
- C/C++ 编译器（GCC 或 Clang）
- Git

### 安装

1. 克隆此仓库：
```bash
git clone https://github.com/yourusername/LC76G_Pico.git
cd LC76G_Pico
```

2. 设置 Pico SDK（如果尚未完成）：
```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

### 在 Linux/macOS 上构建

3. 构建项目：
```bash
mkdir build
cd build
cmake ..
make
```

### 在 Windows 上构建

3. 在 Windows 上设置 Pico SDK 路径：
```cmd
set PICO_SDK_PATH=C:\path\to\pico-sdk
```

4. 选项1：使用提供的批处理文件构建：
```cmd
build_pico.bat
```
此脚本将自动：
- 清理之前的构建文件
- 创建构建目录
- 使用 MinGW Makefiles 通过 CMake 配置项目
- 使用 MinGW Make 构建项目

5. 选项2：在 Windows 上手动构建：
```cmd
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make -j8
```

> **注意**：确保已安装 MinGW 并将其添加到系统 PATH 中。对于 Windows，建议使用[官方 Raspberry Pi Pico 设置指南](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)配置您的开发环境。

### 上传到 Pico

构建项目后，上传到您的 Pico：
   - 按住 Pico 上的 BOOTSEL 按钮
   - 按住按钮的同时将其连接到计算机
   - 连接后释放按钮
   - 将生成的 `.uf2` 文件（例如 `gps_example.uf2`）复制到挂载的 Pico 驱动器

### 使用示例

基本示例演示了接收和处理 GPS 数据：

```c
#include "gps/gps_parser.h"

int main() {
    stdio_init_all();
    
    // 初始化 GPS 模块（使用默认的 UART0 配置：GPIO0-TX, GPIO1-RX）
    gps_init(GPS_DEFAULT_UART_ID, GPS_DEFAULT_BAUD_RATE, 
             GPS_DEFAULT_TX_PIN, GPS_DEFAULT_RX_PIN, -1, -1);
    
    while (true) {
        // 解析接收到的 GPS 数据
        if (gps_parse_data()) {
            // 获取处理过的 GPS 数据
            gps_data_t data = gps_get_data();
            
            // 使用数据做一些操作
            printf("位置: %.6f%c, %.6f%c\n", 
                   fabsf(data.latitude), data.lat_dir,
                   fabsf(data.longitude), data.lon_dir);
        }
        
        sleep_ms(100);
    }
    
    return 0;
}
```

## API 参考

### 核心函数

| 函数 | 描述 |
|----------|-------------|
| `gps_init()` | 初始化 GPS 模块 |
| `gps_parse_data()` | 处理接收到的 NMEA 语句 |
| `gps_get_data()` | 获取当前 GPS 数据结构 |
| `gps_get_position()` | 获取纬度和经度值 |
| `gps_has_fix()` | 检查是否有有效的定位数据 |

### 专用函数

| 函数 | 描述 |
|----------|-------------|
| `gps_get_baidu_coordinates()` | 转换为百度地图格式 |
| `gps_get_google_coordinates()` | 转换为谷歌地图格式 |
| `gps_hot_start()` | 执行快速 GPS 重启 |
| `gps_cold_start()` | 执行完全 GPS 重置 |
| `gps_set_standby_mode()` | 进入或退出低功耗模式 |

## 贡献

欢迎贡献！请随时提交 Pull Request。

1. Fork 项目
2. 创建您的特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交您的更改 (`git commit -m 'Add some amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 打开一个 Pull Request

## 许可证

该项目使用 MIT 许可证 - 有关详细信息，请参阅 [LICENSE](LICENSE) 文件。

## 致谢

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [NMEA 标准](https://www.nmea.org/)
- [WGS-84 到 GCJ-02 的转换](https://github.com/googollee/eviltransform) 