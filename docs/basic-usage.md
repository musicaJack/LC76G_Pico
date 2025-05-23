# 基本使用

本文档介绍 LC76G_Pico GPS 跟踪器的基本使用方法和功能。

## 设备启动

将 LC76G_Pico 设备连接到电源（通过 USB 或外部电源）后，设备将自动启动：

1. **初始化阶段**：
   - 设备上电后，LCD 屏幕将显示启动画面
   - 系统会初始化 GPS 模块和显示屏
   - 初始化完成后，显示屏将切换到"L76X GPS Monitor"主界面

2. **GPS 信号获取**：
   - 首次启动时，GPS 模块需要一段时间获取卫星信号（冷启动可能需要 30 秒至几分钟）
   - 在获取有效定位前，屏幕将显示零值坐标或"等待定位..."信息
   - 卫星信号状态在屏幕右上角通过指示灯和屏幕底部通过信号强度图显示

## 主界面功能

LC76G_Pico 显示屏的主界面分为几个功能区域：

### 顶部标题栏

显示标题信息：
- "L76X GPS Monitor"标题
- 右上角状态指示灯（蓝色/闪烁绿色表示正常定位，闪烁黄色表示等待定位）

### 主信息区

显示当前 GPS 数据：
- 原始 WGS-84 经纬度坐标
- 百度地图经纬度坐标（BD-09）
- 谷歌地图经纬度坐标（GCJ-02）
- 速度（km/h）
- 航向（度）
- 海拔高度（米）
- 日期（YYYY-MM-DD 格式）
- 时间（HH:MM:SS 格式）

### 卫星信号区

位于屏幕底部，显示：
- "Satellite Signal"标题
- 卫星信号强度柱状图（模拟值）
- 当未定位时显示空信号框

## 使用场景

### 基本定位模式

最简单的使用方式是作为位置显示设备：

1. 将设备放置在视野良好的位置（室外或靠近窗户）
2. 等待 GPS 模块获取定位信号
3. 查看屏幕上显示的位置信息

### 速度监测模式

在移动中使用设备：

1. 将设备安装在车辆、自行车或其他移动物体上
2. 确保 GPS 天线部分朝向天空
3. 设备将实时显示速度和方向信息

### 坐标转换功能

设备自动支持多种坐标系统转换：

1. 获取当前 GPS 位置（WGS-84原始坐标）
2. 自动转换并同时显示：
   - 百度地图坐标（BD-09）
   - 谷歌/高德地图坐标（GCJ-02）

## 数据查看方式

### 通过显示屏查看

显示屏是查看数据的主要方式，所有重要信息都会在屏幕上直观显示。屏幕每秒更新一次数据。

### 通过串口监视

对于开发人员或需要更详细数据的用户：

1. 通过 USB 将设备连接到计算机
2. 使用串口终端软件（如 PuTTY、Screen 或 Arduino IDE 的串口监视器）
3. 配置正确的串口和波特率（通常为 115200）
4. 查看输出的详细 GPS 数据和系统状态信息
5. 启用调试模式可以查看更多详细信息（通过修改代码中的 `enable_debug` 变量）

## NMEA 输出配置

LC76G_Pico 使用以下 NMEA 配置：

1. **输出语句类型**：
   - 使用 RMC 和 GGA 语句（`$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0`）
   - RMC 语句提供速度和方向信息
   - GGA 语句提供高度和定位质量信息

2. **更新频率**：
   - 配置为 1Hz（`$PMTK220,1000`），即每秒一次更新

## 电源管理

LC76G_Pico 的电源管理功能：

1. **低功耗设计**：
   - 代码优化以减少功耗
   - 适当的延时以平衡性能和功耗

2. **备份模式**：
   - GPS 模块支持备份（低功耗）模式
   - 代码提供了 `vendor_gps_enter_backup_mode` 和 `vendor_gps_exit_backup_mode` 函数用于控制备份模式
   - 此功能可用于降低设备功耗，特别是在长时间静止使用时

> 注意：当前默认实现中没有提供自动切换到省电模式的功能，也没有实现背光控制或自动休眠功能。用户可以通过修改代码来实现这些功能。

## 常见操作问题

### GPS 无法定位

如果 GPS 长时间无法获取定位：

1. 确保设备处于室外或窗户附近，天空视野开阔
2. 避免在高楼之间、隧道内或地下室使用
3. 首次使用时，冷启动可能需要较长时间（5-10 分钟）
4. 检查 GPS 天线部分是否被金属物体覆盖
5. 查看右上角状态指示灯：如果一直呈黄色闪烁，表示尚未获得有效定位

### 显示屏问题

如果显示屏出现异常：

1. **屏幕无显示**：
   - 检查电源连接
   - 确认硬件连接是否正确
   - 尝试重启设备

2. **显示不清晰或错乱**：
   - 重启设备
   - 检查是否受到强电磁干扰
   - 在极端温度下可能影响显示性能

### 数据异常

如果 GPS 数据明显不准确：

1. 移动到更开阔的区域重新获取信号
2. 等待一段时间让设备收集更多卫星数据提高精度
3. 检查日期和时间是否正确（不正确可能导致位置计算错误）
4. 重启设备，让 GPS 模块重新获取卫星数据
5. 如果仅时间准确但坐标不正确，请等待直到右上角状态指示灯变为绿色闪烁

## 实用技巧

### 安装建议

1. 将设备安装在视野开阔处，GPS 天线部分尽量朝向天空
2. 避免将设备放置在有金属屏蔽或强电磁干扰的区域
3. 对于车载使用，推荐安装在前挡风玻璃下方或仪表台上

### 提高精度

1. 首次使用时，让设备在开阔区域静止几分钟，以获取更准确的初始定位
2. 定期更新固件以获得改进的定位算法
3. 在高楼密集区域，精度可能降低，这是 GPS 系统的物理限制 