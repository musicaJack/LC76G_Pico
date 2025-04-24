# GPS 模块

本页面详细介绍了 LC76G_Pico 项目中 GPS 模块的功能和使用方法。

## 概述

LC76G_Pico 项目使用的是 L76X GPS 模块，这是一款高性能的 GNSS（全球导航卫星系统）接收器，支持 GPS、北斗、GLONASS 等多种卫星导航系统。该模块通过 UART 接口与 Raspberry Pi Pico 通信，提供精确的位置、速度和时间信息。

## 技术规格

- **接收器类型**：L76X GNSS 模块
- **支持的导航系统**：GPS, 北斗, GLONASS
- **通信接口**：UART (默认 9600 bps)
- **定位精度**：2.5m CEP (水平)
- **更新频率**：1Hz (标准模式)
- **电源要求**：3.3V

## 工作原理

GPS 模块持续接收来自卫星的信号，并以 NMEA 标准格式输出数据。LC76G_Pico 主要处理以下 NMEA 语句：

- **GNRMC/GPRMC**: 提供位置、速度、时间和日期信息
- **GNGGA/GPGGA**: 提供位置、海拔和卫星信息

系统会解析这些语句，提取所需的地理数据，并根据需要进行坐标系转换和显示。

## 数据结构

GPS 数据使用以下结构保存：

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

## 坐标系统支持

LC76G_Pico 支持以下坐标系统：

1. **WGS-84**：GPS 原始坐标系统，国际标准
2. **GCJ-02**：中国国测局坐标系统，用于谷歌地图等服务
3. **BD-09**：百度坐标系统，用于百度地图服务

系统提供坐标转换功能，可以在这些坐标系统之间进行转换。

## 主要函数

### 初始化函数

```c
bool vendor_gps_init(uint uart_id, uint baud_rate, uint tx_pin, uint rx_pin, int force_pin);
```

该函数初始化 GPS 模块，配置 UART 通信参数。

参数说明：
- `uart_id`：UART 接口 ID (0 表示 UART0)
- `baud_rate`：通信波特率（默认 9600）
- `tx_pin`：发送引脚（连接到 GPS 的 RX）
- `rx_pin`：接收引脚（连接到 GPS 的 TX）
- `force_pin`：控制引脚（可选，设为 -1 表示不使用）

### 数据获取函数

```c
GNRMC vendor_gps_get_gnrmc();
```

该函数从 GPS 模块读取 NMEA 数据，解析成结构化信息，并返回 GNRMC 结构体。

### 控制命令

```c
void vendor_gps_send_command(char *data);
```

该函数向 GPS 模块发送控制命令，如设置更新频率、切换工作模式等。

### 电源管理函数

```c
void vendor_gps_exit_backup_mode(void);
```

该函数使 GPS 模块退出备份（低功耗）模式，恢复正常工作。

## 使用示例

以下是使用 GPS 模块的基本示例：

```c
#include "gps/vendor_gps_parser.h"

int main() {
    stdio_init_all();
    
    // 初始化 GPS 模块（使用默认的 UART0：GPIO0-TX, GPIO1-RX）
    vendor_gps_init(0, 9600, 0, 1, -1);
    
    while (true) {
        // 获取 GPS 数据
        GNRMC gps_data = vendor_gps_get_gnrmc();
        
        // 检查定位是否成功
        if (gps_data.Status) {
            printf("位置: %.6f%c, %.6f%c\n", 
                   gps_data.Lat, gps_data.Lat_area,
                   gps_data.Lon, gps_data.Lon_area);
            printf("速度: %.2f km/h\n", gps_data.Speed);
            printf("航向: %.2f°\n", gps_data.Course);
            printf("时间: %02d:%02d:%02d\n", 
                   gps_data.Time_H, gps_data.Time_M, gps_data.Time_S);
            printf("日期: %s\n", gps_data.Date);
        } else {
            printf("等待定位...\n");
        }
        
        sleep_ms(1000);
    }
    
    return 0;
}
```

## 故障排除

- **无法获取定位**：确保 GPS 模块有良好的天空视野，室内可能无法正常接收卫星信号
- **数据更新缓慢**：首次启动可能需要较长时间（冷启动可能需要几分钟）获取卫星信息
- **通信错误**：检查 UART 连接和波特率设置是否正确
- **坐标精度问题**：在建筑物周围或多径效应严重的环境中，GPS 精度可能下降

## 高级应用

### 坐标转换
```c
// WGS-84 转 GCJ-02 (谷歌地图)
Coordinates wgs84_to_gcj02(double lon, double lat);

// GCJ-02 转 BD-09 (百度地图)
Coordinates gcj02_to_bd09(double lon, double lat);

// WGS-84 转 BD-09 (直接转换到百度地图)
Coordinates wgs84_to_bd09(double lon, double lat);
```

### 低功耗模式
```c
// 进入低功耗模式
void vendor_gps_enter_backup_mode(void);

// 退出低功耗模式
void vendor_gps_exit_backup_mode(void);
```

## 参考资料

- [NMEA 标准](https://www.nmea.org/)
- [L76X GPS 模块数据手册](https://www.quectel.com/)
- [WGS-84 到 GCJ-02 的转换](https://github.com/googollee/eviltransform) 