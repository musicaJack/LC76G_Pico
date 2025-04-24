# 快速入门

本指南将帮助您快速上手 LC76G_Pico GPS 跟踪器项目。

## 先决条件

在开始之前，请确保您具备以下内容：

1. **硬件准备**:
   - Raspberry Pi Pico/Pico W 开发板
   - L76X GPS 模块
   - ST7789 LCD 显示屏（240×320像素）
   - 自开发定制主板（如有）或面包板和连接线
   - USB 数据线（用于供电和编程）

2. **软件准备**:
   - [Raspberry Pi Pico C/C++ SDK](https://github.com/raspberrypi/pico-sdk)
   - CMake（最低版本 3.13）
   - C/C++ 编译器（GCC 或 Clang）
   - Git

## 步骤 1: 获取项目代码

1. 从 GitHub 克隆项目仓库：

```bash
git clone https://github.com/musicaJack/LC76G_Pico.git
cd LC76G_Pico
```

## 步骤 2: 硬件连接

### 方案 A：使用自开发定制主板

如果您有项目的自开发定制主板：

1. 将 Raspberry Pi Pico 安装到定制主板的对应插槽上
2. 确保所有连接正确且牢固
3. 使用 USB 电缆连接到计算机

### 方案 B：使用分立组件

如果您使用分立组件，请按照以下接线图连接：

#### GPS 模块连接

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

#### LCD 显示屏连接

```
┌─────────────────┐                  ┌─────────────────┐
│                 │                  │                 │
│  Raspberry Pi   │                  │   ST7789 LCD    │
│     Pico        │                  │    显示屏       │
│                 │                  │                 │
│  GPIO 19 (MOSI) │─────────────────→│  DIN            │
│  GPIO 18 (SCK)  │─────────────────→│  SCK            │
│  GPIO 17 (CS)   │─────────────────→│  CS             │
│  GPIO 20 (DC)   │─────────────────→│  DC             │
│  GPIO 15 (RST)  │─────────────────→│  RST            │
│  GPIO 10 (BL)   │─────────────────→│  BL             │
│  VCC            │─────────────────→│  VCC            │
│  GND            │←────────────────→│  GND            │
│                 │                  │                 │
└─────────────────┘                  └─────────────────┘
```

## 步骤 3: 环境设置

### 在 Linux/macOS 上设置

1. 设置 Pico SDK 路径：

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

2. 准备构建环境：

```bash
mkdir build
cd build
cmake ..
```

### 在 Windows 上设置

1. 设置 Pico SDK 路径：

```cmd
set PICO_SDK_PATH=C:\path\to\pico-sdk
```

2. 使用提供的批处理文件构建：

```cmd
build_pico.bat
```

或者手动构建：

```cmd
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make -j8
```

## 步骤 4: 编译项目

在构建目录中执行：

```bash
make -j4
```

成功编译后，您应该能看到生成的 `.uf2` 文件，例如 `gps_example.uf2` 或 `vendor_gps_display.uf2`。

## 步骤 5: 上传固件

1. 按住 Pico 上的 BOOTSEL 按钮
2. 按住按钮的同时将 Pico 连接到计算机
3. 连接后释放按钮
4. Pico 将作为一个 USB 存储设备出现在您的计算机上
5. 将编译生成的 `.uf2` 文件复制到此 USB 存储设备
6. 固件上传完成后，Pico 将自动重启并运行新程序

## 步骤 6: 测试运行

### 使用基本示例

如果您上传了 `gps_example.uf2`：

1. 将设备放置在室外或窗户附近，确保 GPS 模块能接收到卫星信号
2. 打开串口监视器（波特率 115200）查看输出的 GPS 数据

### 使用显示屏示例

如果您上传了 `vendor_gps_display.uf2`：

1. 将设备放置在室外或窗户附近，确保 GPS 模块能接收到卫星信号
2. LCD 显示屏将显示 GPS 数据，包括：
   - 当前时间和日期
   - GPS 坐标（WGS-84 格式）
   - 速度和航向信息
   - 转换后的百度和谷歌地图坐标

## 故障排除

如果遇到问题，请检查：

1. **无法编译**:
   - 确保 Pico SDK 路径设置正确
   - 检查是否安装了所有必要的软件依赖

2. **上传问题**:
   - 确保 Pico 处于启动加载模式（BOOTSEL 按钮按下）
   - 检查 USB 连接是否稳定

3. **无 GPS 信号**:
   - 确保 GPS 模块有良好的天空视野
   - 首次启动可能需要几分钟才能获取定位信息
   - 检查接线是否正确

4. **显示屏问题**:
   - 检查 SPI 连接和电源连接
   - 确认背光引脚连接正确

5. **意外重置或不稳定运行**:
   - 检查电源供应是否稳定和充足
   - 可能需要独立电源而非 USB 供电

## 下一步

成功运行基本示例后，您可以：

1. 探索 [API 参考](api-reference.md) 了解更多功能
2. 查看 [系统架构](system-architecture.md) 深入理解项目结构
3. 阅读 [GPS 模块](gps-module.md) 和 [显示模块](display-module.md) 文档了解详细信息
4. 考虑进行定制开发，添加新功能或改进现有功能

如有任何问题，请通过以下方式获取帮助：

- 发送邮件至：yinyue@beingdigital.cn
- 在项目 [Wiki](https://github.com/musicaJack/LC76G_Pico/wiki) 或 [论坛](https://github.com/musicaJack/LC76G_Pico/discussions) 中提问 