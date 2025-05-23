# 坐标系统

本文档介绍 LC76G_Pico 项目中使用的不同地理坐标系统及其转换方法。在中国使用地图服务时，了解不同坐标系统及其差异非常重要，这将帮助您正确显示和使用 GPS 获取的位置数据。

## 目录

- [常见坐标系统](#常见坐标系统)
- [坐标系统差异](#坐标系统差异)
- [坐标转换算法](#坐标转换算法)
- [使用示例](#使用示例)
- [常见问题](#常见问题)

## 常见坐标系统

### WGS-84 (World Geodetic System 1984)

**简介**：全球地理坐标系统，由美国国防部制定，GPS 接收器获取的原始坐标都是基于 WGS-84。

**特点**：
- 全球统一的坐标系统
- GPS 模块直接输出的坐标格式
- 在中国大陆地区，不能直接用于标准地图显示（偏移约 300-500 米）

**用途**：
- 国际航空、航海导航
- 全球定位系统 (GPS) 的基础坐标系统
- 海外地图服务（如国际版 Google Maps）

### GCJ-02 (国测局坐标系)

**简介**：也称为"火星坐标系"，是中国国家测绘局制定的地理坐标系统，在 WGS-84 基础上加入了非线性变换。

**特点**：
- 相对 WGS-84 存在非线性偏移
- 偏移算法不公开，但已被逆向工程还原
- 中国大陆地区合法地图必须使用此坐标系

**用途**：
- 高德地图、腾讯地图
- 大部分国内 Android 设备的地图应用
- Google Maps 中国版

### BD-09 (百度坐标系)

**简介**：百度地图使用的坐标系统，在 GCJ-02 基础上再次加密偏移。

**特点**：
- 在 GCJ-02 基础上再次进行偏移
- 偏移量比 GCJ-02 更大
- 仅百度地图及其 API 使用

**用途**：
- 百度地图及其相关服务
- 使用百度地图 SDK 的应用程序

## 坐标系统差异

### 偏移示例

以北京天安门广场为例（实际 WGS-84 坐标：39.90895, 116.39742）：

| 坐标系统 | 纬度 | 经度 | 与 WGS-84 偏移 |
|---------|------|------|---------------|
| WGS-84  | 39.90895 | 116.39742 | 0 米 |
| GCJ-02  | 39.91024 | 116.40358 | 约 500 米 |
| BD-09   | 39.91639 | 116.41029 | 约 1100 米 |

### 实际影响

- 如果将 GPS 获取的 WGS-84 坐标直接在国内地图上显示，会出现位置偏移
- 使用不同地图服务时，如不进行正确转换，可能导致定位误差
- 导航路径规划会因坐标系不匹配而出现偏差

## 坐标转换算法

LC76G_Pico 项目实现了以下坐标转换API：

### 获取谷歌地图坐标（WGS-84 到 GCJ-02）

```c
Coordinates vendor_gps_get_google_coordinates(void);
```

**功能说明**：
获取当前 GPS 定位位置转换后的谷歌/高德地图坐标（GCJ-02格式）。

**使用方法**：
- 无需提供参数，函数会自动获取当前GPS定位数据并进行转换
- 如果GPS未定位成功，将返回经纬度均为0的坐标
- 如果定位坐标不在中国大陆范围内，将返回原始WGS-84坐标

### 获取百度地图坐标（WGS-84 到 BD-09）

```c
Coordinates vendor_gps_get_baidu_coordinates(void);
```

**功能说明**：
获取当前 GPS 定位位置转换后的百度地图坐标（BD-09格式）。

**使用方法**：
- 无需提供参数，函数会自动获取当前GPS定位数据并进行转换
- 如果GPS未定位成功，将返回经纬度均为0的坐标
- 如果定位坐标不在中国大陆范围内，将返回原始WGS-84坐标
- 转换过程中会先转为GCJ-02坐标，再转为BD-09坐标

### 内部实现细节

项目内部使用以下静态函数实现坐标转换算法，用户通常不需要直接调用这些函数：

```c
// WGS-84转GCJ-02
static Coordinates wgs84_to_gcj02(double lon, double lat);

// GCJ-02转BD-09
static Coordinates gcj02_to_bd09(double lon, double lat);

// WGS-84直接转BD-09
static Coordinates wgs84_to_bd09(double lon, double lat);
```

**转换原理**：
1. 判断坐标是否在中国大陆范围内（使用经纬度矩形区域判断）
2. 如不在中国大陆范围内，直接返回原始坐标
3. 如在中国大陆范围内，应用非线性偏移算法进行转换
4. 转换涉及复杂的数学计算，包括椭球体参数和偏移系数

## 使用示例

### 基本转换示例

```c
#include "gps/vendor_gps_parser.h"

int main() {
    stdio_init_all();
    
    // 初始化 GPS 模块
    vendor_gps_init(0, 9600, 0, 1, -1);
    
    // 等待定位成功
    while (true) {
        GNRMC gps_data = vendor_gps_get_gnrmc();
        
        if (gps_data.Status) {
            // 获取原始WGS-84坐标
            printf("原始坐标: %.6f, %.6f\n", gps_data.Lon, gps_data.Lat);
            
            // 获取谷歌地图坐标
            Coordinates google_coords = vendor_gps_get_google_coordinates();
            printf("谷歌地图坐标: %.6f, %.6f\n", google_coords.Lon, google_coords.Lat);
            
            // 获取百度地图坐标
            Coordinates baidu_coords = vendor_gps_get_baidu_coordinates();
            printf("百度地图坐标: %.6f, %.6f\n", baidu_coords.Lon, baidu_coords.Lat);
            
            break;
        }
        
        sleep_ms(1000);
    }
    
    return 0;
}
```

### 与地图 URL 集成

```c
#include "gps/vendor_gps_parser.h"
#include <stdio.h>

// 生成高德地图链接
void generate_amap_url(void) {
    Coordinates google_coords = vendor_gps_get_google_coordinates();
    char url[256];
    sprintf(url, "https://uri.amap.com/marker?position=%.6f,%.6f", 
            google_coords.Lon, google_coords.Lat);
    printf("高德地图链接: %s\n", url);
}

// 生成百度地图链接
void generate_baidu_url(void) {
    Coordinates baidu_coords = vendor_gps_get_baidu_coordinates();
    char url[256];
    sprintf(url, "https://api.map.baidu.com/marker?location=%.6f,%.6f&title=我的位置", 
            baidu_coords.Lat, baidu_coords.Lon);  // 注意百度地图 API 中经纬度顺序是反的
    printf("百度地图链接: %s\n", url);
}
```

## 常见问题

### Q: 为什么中国地图坐标系统与国际标准不同？

A: 出于国家安全考虑，中国政府规定所有公开地图服务必须使用经过加密偏移的坐标系统，以防止未经授权的精确测绘。

### Q: 我是否可以将 GCJ-02/BD-09 转回 WGS-84？

A: 理论上可以通过逆向算法进行近似转换，但由于原始算法包含随机偏移，因此转换回来会有少量误差。项目目前未实现这些反向转换功能。

### Q: 国内哪些地区不需要进行坐标转换？

A: 中国香港、澳门和台湾地区通常不需要进行坐标转换，可以直接使用 WGS-84 坐标。

### Q: 如何判断一个坐标是否在中国大陆范围内？

A: 项目中使用了简化的矩形判断方法来确定坐标是否在中国大陆范围内，这是坐标转换算法的一部分。如果坐标不在中国大陆范围内，则不进行偏移处理。 