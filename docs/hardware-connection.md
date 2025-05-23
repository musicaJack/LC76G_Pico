# 硬件连接

本页面详细说明了 LC76G_Pico 项目的硬件连接方式。

## 系统硬件组成

LC76G_Pico 项目由以下主要硬件组件组成：

1. **Raspberry Pi Pico/Pico W**：作为主控制器
2. **L76X GPS 模块**：用于 GPS 信号接收和处理
3. **ST7789 LCD 显示屏**：用于显示 GPS 数据和系统状态
4. **自开发定制主板**：作为组件集成基座

> **重要说明**：实际项目使用的是一块自开发的定制主板，该主板已集成上述所有组件。以下接线图适用于自行搭建的场景，同时也是定制主板的设计参考。

## 接线图

### L76X GPS 模块接线

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

### ST7789 LCD 显示屏接线

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

## 接口说明

### GPS 模块接口

L76X GPS 模块通过 UART 接口与 Raspberry Pi Pico 通信：

- **UART 通信**：使用 Pico 的 UART0 (GPIO 0/1)
- **波特率**：默认配置为 9600 bps
- **电源要求**：模块需要 3.3V 电源

### 显示屏接口

ST7789 LCD 显示屏通过 SPI 接口与 Raspberry Pi Pico 通信：

- **SPI 接口**：使用 Pico 的 SPI0
- **时钟速度**：40MHz
- **屏幕分辨率**：240×320 像素
- **显示色彩**：RGB565 格式，16位色彩深度

## 定制主板设计考虑

自开发的定制主板在设计时考虑了以下因素：

1. **紧凑布局**：最小化 PCB 尺寸和组件间距
2. **电源管理**：提供稳定的电源供应和电平转换（如需要）
3. **信号完整性**：SPI 高速信号的走线需要考虑阻抗匹配
4. **扩展性**：预留额外的 GPIO 引脚以供未来扩展使用

## 故障排除

如果遇到硬件连接问题，请检查：

1. **接线是否正确**：确保所有引脚均已正确连接
2. **电源是否稳定**：检查 VCC 和 GND 连接
3. **引脚定义**：确认硬件模块的引脚定义与文档一致
4. **通信参数**：验证软件中配置的通信参数（波特率、SPI 模式等）是否与硬件匹配 