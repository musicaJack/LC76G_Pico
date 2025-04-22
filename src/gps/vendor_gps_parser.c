/**
 * @file vendor_gps_parser.c
 * @brief 厂商提供的GPS解析逻辑的移植实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "gps/vendor_gps_parser.h"

// 使用厂商定义的常量
#define BUFFSIZE 800

// 厂商代码使用的查找表
static const char Temp[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

// 坐标转换常量
static const double pi = 3.14159265358979324;
static const double a = 6378245.0;
static const double ee = 0.00669342162296594323;
static const double x_pi = 3.14159265358979324 * 3000.0 / 180.0;

// GPS UART配置
static uart_inst_t *gps_uart = NULL;
static uint gps_uart_id = 0;
static uint gps_tx_pin = 0;
static uint gps_rx_pin = 0;
static int gps_force_pin = -1;

// 数据缓冲区
static char buff_t[BUFFSIZE] = {0};

// 全局GPS数据
static GNRMC GPS = {0};

// 是否显示详细调试日志
static bool debug_output = false;

/******************************************************************************
function:	
	Latitude conversion
******************************************************************************/
static double transformLat(double x, double y)
{
	double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
    ret += (20.0 * sin(y * pi) + 40.0 * sin(y / 3.0 * pi)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * pi) + 320 * sin(y * pi / 30.0)) * 2.0 / 3.0;
    return ret;
}

/******************************************************************************
function:	
	Longitude conversion
******************************************************************************/
static double transformLon(double x, double y)
{
	double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
    ret += (20.0 * sin(x * pi) + 40.0 * sin(x / 3.0 * pi)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * pi) + 300.0 * sin(x / 30.0 * pi)) * 2.0 / 3.0;
    return ret;
}

/******************************************************************************
function:	
	GCJ-02 international standard converted to Baidu map BD-09 standard
******************************************************************************/
static Coordinates bd_encrypt(Coordinates gg)
{
	Coordinates bd;
    double x = gg.Lon, y = gg.Lat;
	double z = sqrt(x * x + y * y) + 0.00002 * sin(y * x_pi);
	double theta = atan2(y, x) + 0.000003 * cos(x * x_pi);
	bd.Lon = z * cos(theta) + 0.0065;
	bd.Lat = z * sin(theta) + 0.006;
	return bd;
}

/******************************************************************************
function:	
	GPS's WGS-84 standard is converted into GCJ-02 international standard
******************************************************************************/
static Coordinates transform(Coordinates gps)
{
	Coordinates gg;
    double dLat = transformLat(gps.Lon - 105.0, gps.Lat - 35.0);
    double dLon = transformLon(gps.Lon - 105.0, gps.Lat - 35.0);
    double radLat = gps.Lat / 180.0 * pi;
    double magic = sin(radLat);
    magic = 1 - ee * magic * magic;
    double sqrtMagic = sqrt(magic);
    dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * pi);
    dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * pi);
    gg.Lat = gps.Lat + dLat;
    gg.Lon = gps.Lon + dLon;
	return gg;
}

/**
 * @brief 将NMEA格式的经纬度转换为十进制度数格式
 * 
 * 例如: 3113.313760 -> 31.221896度
 * 
 * @param nmea_coord NMEA格式的坐标值
 * @return 十进制度数格式的坐标值
 */
static double convert_nmea_to_decimal(double nmea_coord) {
    int degrees = (int)(nmea_coord / 100.0);
    double minutes = nmea_coord - (degrees * 100.0);
    return degrees + (minutes / 60.0);
}

/**
 * @brief 设置是否输出详细调试日志
 * @param enable 是否启用
 */
void vendor_gps_set_debug(bool enable) {
    debug_output = enable;
}

/**
 * @brief 初始化GPS模块的厂商版本
 */
bool vendor_gps_init(uint uart_id, uint baud_rate, uint tx_pin, uint rx_pin, int force_pin) {
    // 保存UART配置
    gps_uart_id = uart_id;
    gps_tx_pin = tx_pin;
    gps_rx_pin = rx_pin;
    gps_force_pin = force_pin;
    
    // 根据ID获取UART实例
    gps_uart = uart_id == 0 ? uart0 : uart1;
    
    // 初始化UART
    uart_init(gps_uart, baud_rate);
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);
    
    // 配置UART流控制 - 无流控制
    uart_set_hw_flow(gps_uart, false, false);
    
    // 配置UART数据格式 - 8数据位，无奇偶校验，1停止位
    uart_set_format(gps_uart, 8, 1, UART_PARITY_NONE);
    
    // 清除接收缓冲区
    uart_set_fifo_enabled(gps_uart, true);
    
    // 初始化控制引脚（如果可用）
    if (force_pin >= 0) {
        gpio_init(force_pin);
        gpio_set_dir(force_pin, GPIO_OUT);
        gpio_put(force_pin, 0); // 默认：低电平
    }
    
    printf("GPS模块初始化: UART%d, 波特率: %d, TX: %d, RX: %d\n", 
            uart_id, baud_rate, tx_pin, rx_pin);
    
    // 初始化GPS数据
    memset(&GPS, 0, sizeof(GPS));
    
    return true;
}

/**
 * @brief 向GPS模块发送命令
 * @param data 命令字符串，不需要添加校验和
 */
void vendor_gps_send_command(char *data) {
    char Check = data[1], Check_char[3]={0};
    uint8_t i = 0;
    uart_putc(gps_uart, '\r');
    uart_putc(gps_uart, '\n');
    
    // 计算校验和
    for(i=2; data[i] != '\0'; i++){
        Check ^= data[i];
    }
    
    Check_char[0] = Temp[Check/16%16];
    Check_char[1] = Temp[Check%16];
    Check_char[2] = '\0';
   
    // 发送命令
    for(i=0; data[i] != '\0'; i++){
        uart_putc(gps_uart, data[i]);
    }
    
    uart_putc(gps_uart, '*');
    uart_putc(gps_uart, Check_char[0]);
    uart_putc(gps_uart, Check_char[1]);
    uart_putc(gps_uart, '\r');
    uart_putc(gps_uart, '\n');
    
    if (debug_output) {
        printf("发送GPS命令: %s*%s\n", data, Check_char);
    }
}

/**
 * @brief 让GPS模块退出备份模式
 */
void vendor_gps_exit_backup_mode() {
    if (gps_force_pin >= 0) {
        gpio_put(gps_force_pin, 1);
        sleep_ms(1000);
        gpio_put(gps_force_pin, 0);
    }
}

/**
 * @brief 从UART读取一定数量的字节
 * @param data 保存数据的缓冲区
 * @param Num 要读取的字节数
 */
static void vendor_uart_receive_string(char *data, uint16_t Num) {
    uint16_t i = 0;
    
    // 设置超时时间以避免无限等待
    absolute_time_t timeout = make_timeout_time_ms(1000);
    
    while (i < Num - 1) {
        if (uart_is_readable(gps_uart)) {
            data[i] = uart_getc(gps_uart);
            i++;
        }
        
        // 检查超时
        if (time_reached(timeout)) {
            break;
        }
        
        // 短暂休眠以避免忙等待
        sleep_us(10);
    }
    
    data[i] = '\0'; // 确保字符串以null结尾
}

/******************************************************************************
function:	
	Analyze GNRMC data in L76x, latitude and longitude, time
    
    按照NMEA标准，$GNRMC语句格式为：
    $GNRMC,hhmmss.sss,A,BBBB.BBBB,N,CCCCC.CCCC,E,D.D,EEE.E,ddmmyy,,,A*XX
    
    其中：
    - hhmmss.sss 是UTC时间
    - A 是定位状态，A=有效，V=无效
    - BBBB.BBBB 是纬度，格式为ddmm.mmmm（度分格式）
    - N 是纬度方向，N=北，S=南
    - CCCCC.CCCC 是经度，格式为dddmm.mmmm（度分格式）  
    - E 是经度方向，E=东，W=西
    - D.D 是地面速度（节）
    - EEE.E 是地面航向（度）
    - ddmmyy 是UTC日期
    - A 是模式指示
    - XX 是校验和
******************************************************************************/
GNRMC vendor_gps_get_gnrmc() {
    char *token;
    char *rmc_start = NULL;
    char rmc_line[128] = {0};
    
    // 重置GPS状态
    GPS.Status = 0;
    GPS.Time_H = 0;
    GPS.Time_M = 0;
    GPS.Time_S = 0;
    GPS.Speed = 0.0;
    GPS.Course = 0.0;
    
    // 清除经纬度
    GPS.Lat = 0.0;
    GPS.Lon = 0.0;
    
    // 使用厂商原始代码的接收方式获取NMEA数据流
    vendor_uart_receive_string(buff_t, BUFFSIZE);
    
    // 如果开启调试，打印接收到的数据（但只显示前100个字符，以减少日志量）
    if (debug_output) {
        char preview[101] = {0};
        strncpy(preview, buff_t, 100);
        printf("GPS数据前100字符: %s\n", preview);
    }
    
    // 查找GNRMC语句
    rmc_start = strstr(buff_t, "$GNRMC");
    if (!rmc_start) {
        rmc_start = strstr(buff_t, "$GPRMC"); // 尝试查找GPRMC
    }
    
    if (!rmc_start) {
        if (debug_output) {
            printf("未找到RMC语句\n");
        }
        return GPS; // 未找到RMC语句，返回空数据
    }
    
    // 提取完整的RMC语句
    int i = 0;
    while (rmc_start[i] && rmc_start[i] != '\r' && rmc_start[i] != '\n' && i < 127) {
        rmc_line[i] = rmc_start[i];
        i++;
    }
    rmc_line[i] = '\0';
    
    if (debug_output) {
        printf("解析RMC语句: %s\n", rmc_line);
    }
    
    // 使用strtok解析NMEA语句的各个字段
    token = strtok(rmc_line, ",");
    int field = 0;
    
    while (token != NULL) {
        switch (field) {
            case 0: // RMC标识符
                break;
                
            case 1: // UTC时间
                if (strlen(token) >= 6) {
                    // 直接从字符串中提取时间
                    int time = atoi(token);
                    GPS.Time_H = (time / 10000) + 8; // UTC+8时区
                    GPS.Time_M = (time / 100) % 100;
                    GPS.Time_S = time % 100;
                    
                    // 处理跨天情况
                    if (GPS.Time_H >= 24) {
                        GPS.Time_H -= 24;
                    }
                }
                break;
                
            case 2: // 定位状态
                GPS.Status = (token[0] == 'A') ? 1 : 0;
                break;
                
            case 3: // 纬度 (ddmm.mmmm格式)
                if (strlen(token) > 0) {
                    // 保存原始格式以供参考
                    GPS.Lat_Raw = atof(token);
                    // 转换为十进制度格式
                    GPS.Lat = convert_nmea_to_decimal(GPS.Lat_Raw);
                }
                break;
                
            case 4: // 纬度方向
                if (strlen(token) > 0) {
                    GPS.Lat_area = token[0];
                    // 如果是南纬，纬度为负值
                    if (GPS.Lat_area == 'S') {
                        GPS.Lat = -GPS.Lat;
                    }
                }
                break;
                
            case 5: // 经度 (dddmm.mmmm格式)
                if (strlen(token) > 0) {
                    // 保存原始格式以供参考
                    GPS.Lon_Raw = atof(token);
                    // 转换为十进制度格式
                    GPS.Lon = convert_nmea_to_decimal(GPS.Lon_Raw);
                }
                break;
                
            case 6: // 经度方向
                if (strlen(token) > 0) {
                    GPS.Lon_area = token[0];
                    // 如果是西经，经度为负值
                    if (GPS.Lon_area == 'W') {
                        GPS.Lon = -GPS.Lon;
                    }
                }
                break;
                
            case 7: // 地面速度（节）
                if (strlen(token) > 0) {
                    GPS.Speed = atof(token) * 1.852; // 转换为公里/小时
                }
                break;
                
            case 8: // 地面航向（度）
                if (strlen(token) > 0) {
                    GPS.Course = atof(token);
                }
                break;
                
            case 9: // 日期 (ddmmyy)
                if (strlen(token) >= 6) {
                    // 保存原始日期，格式为DDMMYY
                    int day = (token[0] - '0') * 10 + (token[1] - '0');
                    int month = (token[2] - '0') * 10 + (token[3] - '0');
                    int year = 2000 + (token[4] - '0') * 10 + (token[5] - '0');
                    
                    // 格式化日期字符串
                    sprintf(GPS.Date, "%04d-%02d-%02d", year, month, day);
                }
                break;
        }
        
        token = strtok(NULL, ",");
        field++;
    }
    
    if (debug_output && GPS.Status) {
        printf("GPS定位成功: 纬度=%.6f%c(%.6f°), 经度=%.6f%c(%.6f°)\n", 
              GPS.Lat_Raw, GPS.Lat_area, GPS.Lat,
              GPS.Lon_Raw, GPS.Lon_area, GPS.Lon);
    }
    
    return GPS;
}

/******************************************************************************
function:	
	Convert GPS latitude and longitude into Baidu map coordinates
    直接使用已转换的十进制度格式坐标
******************************************************************************/
Coordinates vendor_gps_get_baidu_coordinates() {
    Coordinates temp;
    
    // 直接使用转换后的十进制坐标
    temp.Lat = GPS.Lat;
    temp.Lon = GPS.Lon;
    
    // 地图坐标系转换
    temp = transform(temp);
    temp = bd_encrypt(temp);
    return temp;
}

/******************************************************************************
function:	
	Convert GPS latitude and longitude into Google Maps coordinates
    直接使用已转换的十进制度格式坐标
******************************************************************************/
Coordinates vendor_gps_get_google_coordinates() {
    Coordinates temp;
    
    // 直接使用转换后的十进制坐标
    temp.Lat = GPS.Lat;
    temp.Lon = GPS.Lon;
    
    // 地图坐标系转换
    temp = transform(temp);
    return temp;
} 