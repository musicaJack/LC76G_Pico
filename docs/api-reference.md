# API 参考

本文档详细列出了 LC76G_Pico 项目提供的所有主要 API 函数和数据结构，供开发者参考使用。

## 目录

- [GPS 模块 API](#gps-模块-api)
- [显示模块 API](#显示模块-api)
- [坐标转换 API](#坐标转换-api)
- [数据结构](#数据结构)
- [常量定义](#常量定义)

## GPS 模块 API

### 初始化与配置

#### `vendor_gps_init`

```c
bool vendor_gps_init(uint uart_id, uint baud_rate, uint tx_pin, uint rx_pin, int force_pin);
```

**功能**：初始化 GPS 模块

**参数**：
- `uart_id`：UART 接口 ID (0 表示 UART0, 1 表示 UART1)
- `baud_rate`：通信波特率（默认 9600）
- `tx_pin`：发送引脚（连接到 GPS 的 RX）
- `rx_pin`：接收引脚（连接到 GPS 的 TX）
- `force_pin`：控制引脚（可选，设为 -1 表示不使用）

**返回值**：初始化是否成功

**示例**：
```c
// 使用 UART0，波特率 9600，TX 连接 GPIO 0，RX 连接 GPIO 1
if (!vendor_gps_init(0, 9600, 0, 1, -1)) {
    printf("GPS 初始化失败\n");
    return false;
}
```

#### `vendor_gps_set_debug`

```c
void vendor_gps_set_debug(bool enable);
```

**功能**：启用或禁用 GPS 模块的调试输出

**参数**：
- `enable`：是否启用调试输出

**返回值**：无

### 数据获取

#### `vendor_gps_get_gnrmc`

```c
GNRMC vendor_gps_get_gnrmc(void);
```

**功能**：获取当前 GPS 数据

**参数**：无

**返回值**：包含 GPS 信息的 GNRMC 结构体

**示例**：
```c
GNRMC gps_data = vendor_gps_get_gnrmc();
if (gps_data.Status) {
    printf("经度: %.6f%c\n", gps_data.Lon, gps_data.Lon_area);
    printf("纬度: %.6f%c\n", gps_data.Lat, gps_data.Lat_area);
}
```

### 控制命令

#### `vendor_gps_send_command`

```c
void vendor_gps_send_command(char *data);
```

**功能**：向 GPS 模块发送控制命令

**参数**：
- `data`：命令字符串，无需添加校验和

**返回值**：无

**示例**：
```c
// 发送冷启动命令
vendor_gps_send_command("$PCAS01,0*1C");
```

#### `vendor_gps_exit_backup_mode`

```c
void vendor_gps_exit_backup_mode(void);
```

**功能**：使 GPS 模块退出备份（低功耗）模式

**参数**：无

**返回值**：无

## 显示模块 API

### 初始化与配置

#### `st7789_init`

```c
bool st7789_init(const st7789_config_t *config);
```

**功能**：初始化 ST7789 LCD 驱动

**参数**：
- `config`：显示屏配置参数指针

**返回值**：初始化是否成功

**示例**：
```c
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

if (!st7789_init(&config)) {
    printf("LCD 初始化失败\n");
    return false;
}
```

#### `st7789_set_backlight`

```c
void st7789_set_backlight(bool on);
```

**功能**：控制 LCD 背光

**参数**：
- `on`：背光状态（true 表示开启，false 表示关闭）

**返回值**：无

#### `st7789_set_rotation`

```c
void st7789_set_rotation(uint8_t rotation);
```

**功能**：设置显示方向

**参数**：
- `rotation`：旋转角度（0-3，分别对应 0°、90°、180°、270°）

**返回值**：无

### 绘图函数

#### `st7789_fill_screen`

```c
void st7789_fill_screen(uint16_t color);
```

**功能**：填充整个屏幕为指定颜色

**参数**：
- `color`：填充颜色（RGB565 格式）

**返回值**：无

#### `st7789_draw_pixel`

```c
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
```

**功能**：绘制单个像素

**参数**：
- `x`：X 坐标
- `y`：Y 坐标
- `color`：像素颜色（RGB565 格式）

**返回值**：无

#### `st7789_set_window`

```c
void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
```

**功能**：设置绘图窗口区域

**参数**：
- `x0`：起始 X 坐标
- `y0`：起始 Y 坐标
- `x1`：结束 X 坐标
- `y1`：结束 Y 坐标

**返回值**：无

### 图形库函数

#### `st7789_draw_line`

```c
void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
```

**功能**：绘制线段

**参数**：
- `x0`：起始 X 坐标
- `y0`：起始 Y 坐标
- `x1`：结束 X 坐标
- `y1`：结束 Y 坐标
- `color`：线条颜色

**返回值**：无

#### `st7789_draw_rect`

```c
void st7789_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
```

**功能**：绘制矩形边框

**参数**：
- `x`：左上角 X 坐标
- `y`：左上角 Y 坐标
- `w`：宽度
- `h`：高度
- `color`：边框颜色

**返回值**：无

#### `st7789_fill_rect`

```c
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
```

**功能**：绘制填充矩形

**参数**：
- `x`：左上角 X 坐标
- `y`：左上角 Y 坐标
- `w`：宽度
- `h`：高度
- `color`：填充颜色

**返回值**：无

#### `st7789_draw_circle`

```c
void st7789_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
```

**功能**：绘制圆形边框

**参数**：
- `x0`：圆心 X 坐标
- `y0`：圆心 Y 坐标
- `r`：半径
- `color`：边框颜色

**返回值**：无

#### `st7789_fill_circle`

```c
void st7789_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
```

**功能**：绘制填充圆形

**参数**：
- `x0`：圆心 X 坐标
- `y0`：圆心 Y 坐标
- `r`：半径
- `color`：填充颜色

**返回值**：无

## 坐标转换 API

#### `wgs84_to_gcj02`

```c
Coordinates wgs84_to_gcj02(double lon, double lat);
```

**功能**：将 WGS-84 坐标转换为 GCJ-02 坐标（谷歌/高德地图）

**参数**：
- `lon`：WGS-84 经度
- `lat`：WGS-84 纬度

**返回值**：GCJ-02 坐标结构体

#### `gcj02_to_bd09`

```c
Coordinates gcj02_to_bd09(double lon, double lat);
```

**功能**：将 GCJ-02 坐标转换为 BD-09 坐标（百度地图）

**参数**：
- `lon`：GCJ-02 经度
- `lat`：GCJ-02 纬度

**返回值**：BD-09 坐标结构体

#### `wgs84_to_bd09`

```c
Coordinates wgs84_to_bd09(double lon, double lat);
```

**功能**：将 WGS-84 坐标直接转换为 BD-09 坐标

**参数**：
- `lon`：WGS-84 经度
- `lat`：WGS-84 纬度

**返回值**：BD-09 坐标结构体

## 数据结构

### GPS 数据结构

#### `GNRMC`

```c
typedef struct {
    // 基本位置信息
    double Lon;         // 经度（十进制度格式）
    double Lat;         // 纬度（十进制度格式）
    char Lon_area;      // 经度区域 ('E'/'W')
    char Lat_area;      // 纬度区域 ('N'/'S')
    
    // 时间信息
    uint8_t Time_H;     // 小时
    uint8_t Time_M;     // 分钟
    uint8_t Time_S;     // 秒
    char Date[11];      // 日期（YYYY-MM-DD格式）
    
    // 状态信息
    uint8_t Status;     // 定位状态 (1:定位成功 0:定位失败)
    
    // 原始格式数据
    double Lon_Raw;     // 原始NMEA格式经度（dddmm.mmmm）
    double Lat_Raw;     // 原始NMEA格式纬度（ddmm.mmmm）
    
    // 扩展信息
    double Speed;       // 速度（公里/小时）
    double Course;      // 航向（度）
    double Altitude;    // 海拔高度（米）
} GNRMC;
```

#### `Coordinates`

```c
typedef struct {
    double Lon;         // 经度
    double Lat;         // 纬度
} Coordinates;
```

### 显示屏配置结构

#### `st7789_config_t`

```c
typedef struct {
    spi_inst_t *spi_inst;    // SPI 实例
    uint32_t spi_speed_hz;   // SPI 速度 (Hz)
    
    // 引脚定义
    uint8_t pin_din;         // MOSI
    uint8_t pin_sck;         // SCK
    uint8_t pin_cs;          // CS
    uint8_t pin_dc;          // 数据/命令
    uint8_t pin_reset;       // 复位
    uint8_t pin_bl;          // 背光
    
    // 屏幕参数
    uint16_t width;          // 宽度
    uint16_t height;         // 高度
    
    // 方向
    uint8_t rotation;        // 旋转
} st7789_config_t;
```

## 常量定义

### GPS 相关常量

```c
// UART 默认参数
#define GPS_DEFAULT_UART_ID     0
#define GPS_DEFAULT_BAUD_RATE   9600
#define GPS_DEFAULT_TX_PIN      0
#define GPS_DEFAULT_RX_PIN      1
```

### 显示屏颜色常量

```c
// 颜色定义 (RGB565 格式)
#define ST7789_BLACK       0x0000
#define ST7789_WHITE       0xFFFF
#define ST7789_RED         0xF800
#define ST7789_GREEN       0x07E0
#define ST7789_BLUE        0x001F
#define ST7789_YELLOW      0xFFE0
#define ST7789_CYAN        0x07FF
#define ST7789_MAGENTA     0xF81F
```

## 使用示例

### 完整的 GPS 显示示例

```c
#include "pico/stdlib.h"
#include "gps/vendor_gps_parser.h"
#include "display/st7789.h"
#include "display/st7789_gfx.h"

int main() {
    stdio_init_all();
    
    // 初始化 GPS 模块
    vendor_gps_init(0, 9600, 0, 1, -1);
    
    // 初始化显示屏
    st7789_config_t config = {
        .spi_inst    = spi0,
        .spi_speed_hz = 40 * 1000 * 1000,
        .pin_din     = 19,
        .pin_sck     = 18,
        .pin_cs      = 17,
        .pin_dc      = 20,
        .pin_reset   = 15,
        .pin_bl      = 10,
        .width       = 240,
        .height      = 320,
        .rotation    = 2  // 旋转 180 度
    };
    
    st7789_init(&config);
    st7789_set_backlight(true);
    st7789_fill_screen(ST7789_BLACK);
    
    while (true) {
        // 获取 GPS 数据
        GNRMC gps_data = vendor_gps_get_gnrmc();
        
        // 清除显示区域
        st7789_fill_rect(10, 50, 220, 200, ST7789_BLACK);
        
        // 显示时间和日期
        char time_str[32];
        sprintf(time_str, "%02d:%02d:%02d  %s", 
                gps_data.Time_H, gps_data.Time_M, gps_data.Time_S, gps_data.Date);
        // ... 在这里添加文本绘制代码
        
        // 显示坐标
        if (gps_data.Status) {
            char pos_str[64];
            sprintf(pos_str, "Lat: %.6f%c  Lon: %.6f%c",
                    gps_data.Lat, gps_data.Lat_area,
                    gps_data.Lon, gps_data.Lon_area);
            // ... 在这里添加文本绘制代码
            
            // 转换坐标并显示
            Coordinates gcj = wgs84_to_gcj02(gps_data.Lon, gps_data.Lat);
            Coordinates bd = gcj02_to_bd09(gcj.Lon, gcj.Lat);
            
            // ... 显示转换后的坐标
        } else {
            // 显示"等待定位"
            // ... 文本绘制代码
        }
        
        sleep_ms(500);  // 刷新频率 2Hz
    }
    
    return 0;
}
``` 