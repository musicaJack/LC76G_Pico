# 显示模块

本页面详细介绍了 LC76G_Pico 项目中显示模块的功能和使用方法。

## 概述

LC76G_Pico 项目使用的是 ST7789 LCD 显示屏，这是一款基于 SPI 接口的彩色显示屏，分辨率为 240×320 像素，支持 16 位色彩（RGB565 格式）。该显示屏用于展示 GPS 数据、卫星状态以及系统信息，提供直观的可视化界面。

## 技术规格

- **显示器类型**：ST7789 TFT LCD
- **分辨率**：240×320 像素
- **色彩深度**：RGB565 格式（65K 色）
- **通信接口**：SPI（最高支持 40MHz）
- **视角**：全视角
- **电源要求**：3.3V

## 硬件架构

显示模块的驱动架构分为三层：

1. **硬件抽象层（HAL）**：负责底层硬件通信，包括 SPI 传输和 GPIO 控制
2. **驱动层**：实现 ST7789 控制器的指令集和基本操作
3. **图形层**：提供绘制基本图形元素的功能

## 初始化配置

以下是 ST7789 显示屏的初始化配置结构：

```c
typedef struct {
    spi_inst_t *spi_inst;    // SPI 实例 (SPI0 或 SPI1)
    uint32_t spi_speed_hz;   // SPI 速度 (Hz)
    
    // 引脚定义
    uint8_t pin_din;         // MOSI 引脚
    uint8_t pin_sck;         // SCK 引脚
    uint8_t pin_cs;          // CS 引脚
    uint8_t pin_dc;          // 数据/命令 引脚
    uint8_t pin_reset;       // 复位引脚
    uint8_t pin_bl;          // 背光引脚
    
    // 屏幕参数
    uint16_t width;          // 宽度
    uint16_t height;         // 高度
    
    // 方向
    uint8_t rotation;        // 旋转 (0-3)
} st7789_config_t;
```

## 预定义颜色常量

显示模块提供了常用颜色的预定义常量：

```c
#define ST7789_BLACK       0x0000   // 黑色
#define ST7789_WHITE       0xFFFF   // 白色
#define ST7789_RED         0xF800   // 红色
#define ST7789_GREEN       0x07E0   // 绿色
#define ST7789_BLUE        0x001F   // 蓝色
#define ST7789_YELLOW      0xFFE0   // 黄色
#define ST7789_CYAN        0x07FF   // 青色
#define ST7789_MAGENTA     0xF81F   // 品红色
```

## 主要函数

### 初始化和配置函数

```c
// 初始化 ST7789 驱动
bool st7789_init(const st7789_config_t *config);

// 设置背光
void st7789_set_backlight(bool on);

// 设置显示方向
void st7789_set_rotation(uint8_t rotation);
```

### 基本绘图函数

```c
// 绘制单个像素
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

// 填充整个屏幕
void st7789_fill_screen(uint16_t color);

// 设置绘图窗口
void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

// 发送块数据到 LCD
void st7789_write_data_buffer(const uint8_t *data, size_t len);
```

### 图形函数

```c
// 绘制水平线
void st7789_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);

// 绘制垂直线
void st7789_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);

// 绘制线段
void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

// 绘制矩形
void st7789_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

// 绘制填充矩形
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

// 绘制圆形
void st7789_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

// 绘制填充圆形
void st7789_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
```

## 用户界面设计

LC76G_Pico 项目使用 ST7789 显示屏实现了以下 UI 组件：

1. **状态栏**：显示时间、日期和卫星状态
2. **坐标显示区**：显示当前坐标（WGS-84 格式）
3. **速度和航向区**：显示当前移动速度和方向
4. **地图坐标区**：显示转换后的百度地图和谷歌地图坐标
5. **系统信息区**：显示系统状态和其他辅助信息

## 使用示例

以下是使用显示模块的基本示例：

```c
#include "display/st7789.h"
#include "display/st7789_gfx.h"

int main() {
    stdio_init_all();
    
    // 初始化 ST7789 显示屏
    st7789_config_t config = {
        .spi_inst    = spi0,
        .spi_speed_hz = 40 * 1000 * 1000, // 40MHz
        .pin_din     = 19,
        .pin_sck     = 18,
        .pin_cs      = 17,
        .pin_dc      = 20,
        .pin_reset   = 15,
        .pin_bl      = 10,
        .width       = 240,
        .height      = 320,
        .rotation    = 0
    };
    
    // 初始化显示屏
    st7789_init(&config);
    
    // 设置显示方向（竖屏模式，180度旋转）
    st7789_set_rotation(2);
    
    // 打开背光
    st7789_set_backlight(true);
    
    // 清屏为黑色
    st7789_fill_screen(ST7789_BLACK);
    
    // 绘制基本图形
    st7789_draw_rect(10, 10, 220, 300, ST7789_WHITE);           // 白色边框
    st7789_fill_rect(20, 50, 200, 40, ST7789_BLUE);             // 蓝色填充矩形
    st7789_draw_circle(120, 160, 60, ST7789_YELLOW);            // 黄色圆形
    st7789_draw_line(10, 10, 230, 310, ST7789_RED);             // 红色对角线
    
    while (true) {
        // 主循环
        sleep_ms(1000);
    }
    
    return 0;
}
```

## 显示屏性能优化

为了获得最佳显示性能，请考虑以下几点：

1. **批量更新**：尽可能使用批量更新而不是单像素操作，以减少 SPI 通信开销
2. **窗口设置**：更新前正确设置窗口区域，仅更新需要变化的部分
3. **使用 DMA**：对于大量数据传输，可以考虑使用 DMA 方式
4. **减少刷新频率**：仅在数据变化时更新屏幕，避免不必要的重绘

## 故障排除

如果遇到显示问题，请检查：

1. **接线正确性**：确保所有 SPI 和控制引脚正确连接
2. **SPI 配置**：验证 SPI 模式和时钟设置
3. **初始化顺序**：ST7789 需要特定的初始化命令序列
4. **电源稳定性**：不稳定的电源可能导致显示异常 