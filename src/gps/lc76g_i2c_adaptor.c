/**
 * @file lc76g_i2c_adaptor.c
 * @brief LC76G GPS模块I2C适配器实现 - 基于厂商demo移植到Pico平台
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/mutex.h"
#include "gps/lc76g_i2c_adaptor.h"

// =============================================================================
// 全局变量
// =============================================================================

static i2c_inst_t *g_i2c_inst = NULL;
static uint8_t g_i2c_addr = QL_CRCW_ADDR;
static uint g_sda_pin = 0;
static uint g_scl_pin = 0;
static int g_force_pin = -1;
static bool g_debug_enabled = false;

// 数据缓冲区
static uint8_t g_write_data_buf[1024] = {0};
static uint8_t g_read_data_buf[4097] = {0};

// 命令处理结构
static Ql_GNSS_Command_TypeDef g_command_deal = {0};

// 互斥锁
static mutex_t g_i2c_mutex;
static mutex_t g_write_cmd_mutex;

// GPS数据
static LC76G_GPS_Data g_gps_data = {0};

// 坐标转换常量
static const double pi = 3.14159265358979324;
static const double a = 6378245.0;
static const double ee = 0.00669342162296594323;
static const double x_pi = 3.14159265358979324 * 3000.0 / 180.0;

// =============================================================================
// 内部函数声明
// =============================================================================

static bool write_dummy_addr(uint8_t i2c_addr);
static bool write_cr_data(int reg, int cfg_len, uint8_t *write_buffer);
static bool read_rd_data(int read_len, uint8_t *read_buffer);
static bool write_cw_data(int reg, int cfg_len, uint8_t *write_buffer);
static bool write_wr_data(int write_len, uint8_t *write_buffer);
static int recovery_i2c(void);
static void num2buf_small(int num, uint8_t *buf);
static int buf2num_small(uint8_t *buf);
static bool data_interception(uint8_t *src_string, const char *interception_string, uint8_t *des_string);
static double convert_nmea_to_decimal(double nmea_coord);
static double transform_lat(double x, double y);
static double transform_lon(double x, double y);
static Coordinates bd_encrypt(Coordinates gg);
static Coordinates transform(Coordinates gps);
static void parse_nmea_data(const char *nmea_data, int data_len);
static void parse_rmc_sentence(const char *rmc_line);
static void parse_gga_sentence(const char *gga_line);
static void parse_gsv_sentence(const char *gsv_line);

// =============================================================================
// 工具函数实现
// =============================================================================

static void num2buf_small(int num, uint8_t *buf) {
    for(int i = 0; i < 4; i++) {
        buf[i] = (num >> (8 * i)) & 0xff;
    }
}

static int buf2num_small(uint8_t *buf) {
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

static bool data_interception(uint8_t *src_string, const char *interception_string, uint8_t *des_string) {
    char *pos_get = strstr((char*)src_string, interception_string);
    if(pos_get == NULL) {
        return false;
    }
    int pos = pos_get - (char*)src_string + strlen(interception_string);
    memcpy(des_string, src_string, pos);
    des_string[pos] = '\0';
    return true;
}

static double convert_nmea_to_decimal(double nmea_coord) {
    int degrees = (int)(nmea_coord / 100.0);
    double minutes = nmea_coord - (degrees * 100.0);
    return degrees + (minutes / 60.0);
}

// 坐标转换函数 (WGS84 -> GCJ02)
static double transform_lat(double x, double y) {
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
    ret += (20.0 * sin(y * pi) + 40.0 * sin(y / 3.0 * pi)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * pi) + 320 * sin(y * pi / 30.0)) * 2.0 / 3.0;
    return ret;
}

static double transform_lon(double x, double y) {
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
    ret += (20.0 * sin(x * pi) + 40.0 * sin(x / 3.0 * pi)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * pi) + 300.0 * sin(x / 30.0 * pi)) * 2.0 / 3.0;
    return ret;
}

static Coordinates bd_encrypt(Coordinates gg) {
    Coordinates bd;
    double x = gg.Lon, y = gg.Lat;
    double z = sqrt(x * x + y * y) + 0.00002 * sin(y * x_pi);
    double theta = atan2(y, x) + 0.000003 * cos(x * x_pi);
    bd.Lon = z * cos(theta) + 0.0065;
    bd.Lat = z * sin(theta) + 0.006;
    return bd;
}

static Coordinates transform(Coordinates gps) {
    Coordinates gg;
    double dLat = transform_lat(gps.Lon - 105.0, gps.Lat - 35.0);
    double dLon = transform_lon(gps.Lon - 105.0, gps.Lat - 35.0);
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

// =============================================================================
// I2C通信函数实现
// =============================================================================

static bool write_dummy_addr(uint8_t i2c_addr) {
    uint8_t dummy_data = 0;
    int result = i2c_write_blocking(g_i2c_inst, i2c_addr, &dummy_data, 1, false);
    
    if(g_debug_enabled) {
        printf("[I2C调试] write_dummy_addr(0x%02X) 结果: %d\n", i2c_addr, result);
    }
    
    return result == 1;  // 成功写入1个字节
}

static bool write_cr_data(int reg, int cfg_len, uint8_t *write_buffer) {
    uint8_t data[8] = {0};
    num2buf_small(reg, data);
    num2buf_small(cfg_len, &data[4]);
    
    int result = i2c_write_blocking(g_i2c_inst, QL_CRCW_ADDR, data, 8, false);
    return result == 8;  // 成功写入8个字节
}

static bool read_rd_data(int read_len, uint8_t *read_buffer) {
    int result = i2c_read_blocking(g_i2c_inst, QL_RD_ADDR, read_buffer, read_len, false);
    return result == read_len;  // 成功读取指定长度的字节
}

static bool write_cw_data(int reg, int cfg_len, uint8_t *write_buffer) {
    uint8_t data[8] = {0};
    num2buf_small(reg, data);
    num2buf_small(cfg_len, &data[4]);
    
    int result = i2c_write_blocking(g_i2c_inst, QL_CRCW_ADDR, data, 8, false);
    return result == 8;  // 成功写入8个字节
}

static bool write_wr_data(int write_len, uint8_t *write_buffer) {
    int result = i2c_write_blocking(g_i2c_inst, QL_WR_ADDR, write_buffer, write_len, false);
    return result == write_len;  // 成功写入指定长度的字节
}

static int recovery_i2c(void) {
    if(write_dummy_addr(QL_CRCW_ADDR)) {
        return 0;
    } else if(write_dummy_addr(QL_RD_ADDR)) {
        if(g_debug_enabled) {
            printf("recovery success, 0x54 dump i2c\n");
        }
        return 1;
    } else if(write_dummy_addr(QL_WR_ADDR)) {
        if(g_debug_enabled) {
            printf("recovery success, 0x58 dump i2c\n");
        }
        return 2;
    } else {
        if(g_debug_enabled) {
            printf("recovery Fail, please check module status\n");
        }
        return -1;
    }
}

// =============================================================================
// 主要I2C通信函数
// =============================================================================

static bool read_data_from_lc76g(uint8_t *data_buf) {
    uint8_t write_data[8] = {0};
    uint8_t read_data[4096] = {0};
    int data_length = 0;
    int total_length = 0;
    
    // 设置从机地址
    g_i2c_addr = QL_CRCW_ADDR;
    
RESTART:
    // 检查0x50地址是否活跃
    for(int i = 0; i < RETRY_TIME; i++) {
        sleep_us(10000);
        if(write_dummy_addr(QL_CRCW_ADDR)) {
            break;
        } else if(i == RETRY_TIME - 1) {
            if(g_debug_enabled) {
                printf("0x50 not alive--%d recovery_i2c\n", i);
            }
            if(recovery_i2c() == -1) {
                return false;
            }
        }
    }
    
    // 配置数据长度寄存器
    for(int i = 0; i < RETRY_TIME; i++) {
        sleep_us(10000);
        if(write_cr_data(QL_CR_REG, QL_CR_LEN, write_data)) {
            memset(write_data, 0, sizeof(write_data));
            break;
        } else if(i == RETRY_TIME - 1) {
            if(g_debug_enabled) {
                printf("0x50 CFG Len not alive--%d\n", i);
            }
            goto RESTART;
        }
        memset(write_data, 0, sizeof(write_data));
    }
    
    // 读取数据长度
    for(int i = 0; i < RETRY_TIME; i++) {
        sleep_us(10000);
        if(read_rd_data(QL_RW_DATA_LENGTH_SIZE, data_buf)) {
            data_length = buf2num_small(data_buf);
            break;
        } else if(i == RETRY_TIME - 1) {
            if(g_debug_enabled) {
                printf("0x54 read not alive--%d\n", i);
            }
            goto RESTART;
        }
    }
    
    if(data_length == 0) {
        if(g_debug_enabled) {
            printf("[原始数据] 数据长度: 0 (无新数据)\n");
        }
        return true; // 没有数据，但不是错误
    } else if(data_length >= 35*1024) {
        if(g_debug_enabled) {
            printf("data len is illegal --- %d\n", data_length);
        }
        return false;
    }
    
    if(g_debug_enabled) {
        printf("[原始数据] 数据长度: %d 字节\n", data_length);
    }
    
    // 读取实际数据
    int remain_length = data_length;
    while(remain_length > 0) {
        if(remain_length > QL_MAX_DATA_LENGTH) {
            data_length = QL_MAX_DATA_LENGTH;
            remain_length -= QL_MAX_DATA_LENGTH;
        } else {
            data_length = remain_length;
            remain_length = 0;
        }
        
        // 配置读数据寄存器
        for(int i = 0; i < RETRY_TIME; i++) {
            sleep_us(10000);
            if(write_cr_data(QL_RD_REG, data_length, write_data)) {
                memset(write_data, 0, sizeof(write_data));
                break;
            } else if(i == RETRY_TIME - 1) {
                if(g_debug_enabled) {
                    printf("0x50 CFG Data not alive--%d\n", i);
                }
            }
            memset(write_data, 0, sizeof(write_data));
        }
        
        // 读取数据
        for(int i = 0; i < RETRY_TIME; i++) {
            sleep_us(10000);
            if(read_rd_data(data_length, read_data)) {
                memcpy(&data_buf[total_length], read_data, data_length);
                total_length += data_length;
                if(total_length >= QL_MAX_DATA_LENGTH) {
                    return true;
                }
                break;
            } else if(i == RETRY_TIME - 1) {
                if(g_debug_enabled) {
                    printf("0x54 read data not alive--%d\n", i);
                }
                return false;
            }
        }
    }
    
    // 打印原始数据内容
    if(g_debug_enabled && total_length > 0) {
        printf("[原始数据] 内容 (%d字节): ", total_length);
        // 限制打印长度，避免输出过长
        int print_len = (total_length > 200) ? 200 : total_length;
        for(int i = 0; i < print_len; i++) {
            if(data_buf[i] >= 32 && data_buf[i] <= 126) {
                printf("%c", data_buf[i]);
            } else {
                printf("\\x%02X", data_buf[i]);
            }
        }
        if(total_length > 200) {
            printf("...(截断)");
        }
        printf("\n");
    }
    
    return true;
}

static bool write_data_to_lc76g(const uint8_t *data_buf, int data_length) {
    int free_length = 0;
    int data_length_temp = data_length;
    uint8_t cw_buf[8] = {0};
    uint8_t free_length_temp[4] = {0};
    bool remain_flag = false;
    
    do {
        g_i2c_addr = QL_CRCW_ADDR;
        
        // 检查0x50地址是否活跃
        for(int i = 0; i < RETRY_TIME; i++) {
            sleep_us(10000);
            if(write_dummy_addr(QL_CRCW_ADDR)) {
                break;
            } else {
                if(recovery_i2c() == -1) {
                    return false;
                }
            }
        }
        
        // 配置读以获取空闲长度
        for(int i = 0; i < RETRY_TIME; i++) {
            sleep_us(10000);
            if(write_cw_data(QL_CW_REG, QL_CW_LEN, cw_buf)) {
                break;
            }
            memset(cw_buf, 0, sizeof(cw_buf));
        }
        
        g_i2c_addr = QL_RD_ADDR;
        for(int i = 0; i < RETRY_TIME; i++) {
            sleep_us(10000);
            if(read_rd_data(QL_CW_LEN, free_length_temp)) {
                free_length = buf2num_small(free_length_temp);
                break;
            }
        }
        
        if(free_length < data_length) {
            remain_flag = true;
            data_length_temp -= free_length;
        } else {
            remain_flag = false;
        }
        
        g_i2c_addr = QL_CRCW_ADDR;
        if(remain_flag) {
            for(int i = 0; i < RETRY_TIME; i++) {
                sleep_us(10000);
                if(write_cw_data(QL_WR_REG, free_length, cw_buf)) {
                    break;
                }
            }
            g_i2c_addr = QL_WR_ADDR;
            sleep_us(10000);
            for(int i = 0; i < RETRY_TIME; i++) {
                if(write_wr_data(free_length, (uint8_t*)data_buf)) {
                    break;
                }
            }
        } else {
            for(int i = 0; i < RETRY_TIME; i++) {
                sleep_us(10000);
                if(write_cw_data(QL_WR_REG, data_length_temp, cw_buf)) {
                    break;
                }
            }
            g_i2c_addr = QL_WR_ADDR;
            sleep_us(10000);
            for(int i = 0; i < RETRY_TIME; i++) {
                if(write_wr_data(data_length_temp, (uint8_t*)&data_buf[data_length - data_length_temp])) {
                    break;
                }
            }
        }
    } while(remain_flag);
    
    return true;
}

// =============================================================================
// 公共API实现
// =============================================================================

bool lc76g_i2c_init(i2c_inst_t *i2c_inst, uint sda_pin, uint scl_pin, uint i2c_speed, int force_pin) {
    g_i2c_inst = i2c_inst;
    g_sda_pin = sda_pin;
    g_scl_pin = scl_pin;
    g_force_pin = force_pin;
    
    // 初始化I2C
    i2c_init(g_i2c_inst, i2c_speed);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    
    // 启用上拉
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    
    // 初始化互斥锁
    mutex_init(&g_i2c_mutex);
    mutex_init(&g_write_cmd_mutex);
    
    // 初始化FORCE引脚
    if(force_pin >= 0) {
        gpio_init(force_pin);
        gpio_set_dir(force_pin, GPIO_OUT);
        gpio_put(force_pin, 0);
    }
    
    // 初始化GPS数据
    memset(&g_gps_data, 0, sizeof(g_gps_data));
    
    // 测试I2C连接
    printf("LC76G I2C适配器初始化成功: I2C%d, SDA: %d, SCL: %d, 速度: %d Hz\n", 
           i2c_inst == i2c0 ? 0 : 1, sda_pin, scl_pin, i2c_speed);
    
    // 测试I2C连接
    printf("[I2C测试] 测试I2C连接...\n");
    if(write_dummy_addr(QL_CRCW_ADDR)) {
        printf("[I2C测试] 0x50地址连接成功\n");
    } else {
        printf("[I2C测试] 0x50地址连接失败\n");
    }
    
    if(write_dummy_addr(QL_RD_ADDR)) {
        printf("[I2C测试] 0x54地址连接成功\n");
    } else {
        printf("[I2C测试] 0x54地址连接失败\n");
    }
    
    if(write_dummy_addr(QL_WR_ADDR)) {
        printf("[I2C测试] 0x58地址连接成功\n");
    } else {
        printf("[I2C测试] 0x58地址连接失败\n");
    }
    
    return true;
}

bool lc76g_send_command(const char *cmd_buf, int cmd_len) {
    if(!cmd_buf || cmd_len <= 0) {
        return false;
    }
    
    mutex_enter_blocking(&g_i2c_mutex);
    bool result = write_data_to_lc76g((const uint8_t*)cmd_buf, cmd_len);
    mutex_exit(&g_i2c_mutex);
    
    if(g_debug_enabled) {
        printf("发送命令: %.*s\n", cmd_len, cmd_buf);
    }
    
    return result;
}

bool lc76g_send_command_and_get_response(const char *cmd_buf, const char *expect_rsp, 
                                        uint32_t timeout_ms, Ql_gnss_command_contx_TypeDef *info) {
    if(!cmd_buf || !expect_rsp || !info) {
        return false;
    }
    
    mutex_enter_blocking(&g_write_cmd_mutex);
    
    // 发送命令
    if(!lc76g_send_command(cmd_buf, strlen(cmd_buf))) {
        mutex_exit(&g_write_cmd_mutex);
        return false;
    }
    
    // 等待响应
    absolute_time_t timeout = make_timeout_time_ms(timeout_ms);
    bool found = false;
    
    while(!time_reached(timeout)) {
        uint8_t data_buf[4096] = {0};
        if(read_data_from_lc76g(data_buf)) {
            char *response = strstr((char*)data_buf, expect_rsp);
            if(response) {
                char line[256] = {0};
                if(data_interception((uint8_t*)response, "\n", (uint8_t*)line)) {
                    if(lc76g_command_get_param(line, strlen(line), info) == No_Error) {
                        found = true;
                        break;
                    }
                }
            }
        }
        sleep_ms(10);
    }
    
    mutex_exit(&g_write_cmd_mutex);
    return found;
}

bool lc76g_read_gps_data(LC76G_GPS_Data *gps_data) {
    if(!gps_data) {
        return false;
    }
    
    mutex_enter_blocking(&g_i2c_mutex);
    
    uint8_t data_buf[4096] = {0};
    bool success = read_data_from_lc76g(data_buf);
    
    if(success && data_buf[0] != 0) {
        parse_nmea_data((char*)data_buf, strlen((char*)data_buf));
        memcpy(gps_data, &g_gps_data, sizeof(LC76G_GPS_Data));
    }
    
    mutex_exit(&g_i2c_mutex);
    
    return success && gps_data->Status;
}

void lc76g_set_debug(bool enable) {
    g_debug_enabled = enable;
}

bool lc76g_wake_i2c(void) {
    uint8_t dummy_data = 0x00;
    return lc76g_send_command((char*)&dummy_data, 1);
}

void lc76g_reset_module(void) {
    if(g_force_pin >= 0) {
        gpio_put(g_force_pin, 1);
        sleep_ms(1000);
        gpio_put(g_force_pin, 0);
    }
}

// =============================================================================
// 命令解析函数实现
// =============================================================================

int32_t lc76g_get_command_checksum(const char *buffer, int32_t buffer_len) {
    const char *ind = buffer;
    uint8_t checkSumL = 0, checkSumR;
    int32_t checksum = 0;
    
    while(ind - buffer < buffer_len) {
        checkSumL ^= *ind;
        ind++;
    }
    
    checkSumR = checkSumL & 0x0F;
    checkSumL = (checkSumL >> 4) & 0x0F;
    checksum = checkSumL * 16;
    checksum = checksum + checkSumR;
    return checksum;
}

uint8_t lc76g_command_get_param(const char *command, int32_t length, Ql_gnss_command_contx_TypeDef *contx) {
    int32_t i, j, k;
    int32_t checksum_l, checksum_r;
    
    if(('$' != command[0]) 
        || ('\n' != command[length - 1]) 
        || ('\r' != command[length - 2]) 
        || ('*' != command[length - 5])) {
        return Format_Error;
    }
    
    for(i = 0, j = 0, k = 0; i < length; i++) {
        if(command[i] > 'z' || command[i] < 0x0A) {
            return Data_Error;
        }
        if((command[i] == ',') || (command[i] == '*')) {
            contx->param[j][k] = 0;
            j++;
            k = 0;
        } else {
            contx->param[j][k] = command[i];
            k++;
        }
    }
    if(k > 30) {
        return Format_Error;       
    }
    contx->param[j][k] = 0;
    contx->param_num = j;
    
    checksum_l = contx->param[j][0] >= 'A' ? contx->param[j][0] - 'A' + 10 : contx->param[j][0] - '0';
    checksum_r = contx->param[j][1] >= 'A' ? contx->param[j][1] - 'A' + 10 : contx->param[j][1] - '0';
    
    contx->checksum = lc76g_get_command_checksum(command + 1, length - 6);
    
    if((checksum_l * 16 + checksum_r) != contx->checksum) {
        if(g_debug_enabled) {
            printf("local check = %d buf check = %d\n", checksum_l * 16 + checksum_r, contx->checksum);
        }
        return CheckSum_Error;
    }
    return No_Error;
}

// =============================================================================
// NMEA数据解析函数实现
// =============================================================================

static void parse_nmea_data(const char *nmea_data, int data_len) {
    if(!nmea_data || data_len <= 0) {
        return;
    }
    
    // 查找RMC句子
    char *rmc_start = strstr(nmea_data, "$GNRMC");
    if(!rmc_start) {
        rmc_start = strstr(nmea_data, "$GPRMC");
    }
    
    if(rmc_start) {
        char rmc_line[128] = {0};
        int i = 0;
        while(rmc_start[i] && rmc_start[i] != '\r' && rmc_start[i] != '\n' && i < 127) {
            rmc_line[i] = rmc_start[i];
            i++;
        }
        rmc_line[i] = '\0';
        parse_rmc_sentence(rmc_line);
    }
    
    // 查找GGA句子
    char *gga_start = strstr(nmea_data, "$GNGGA");
    if(!gga_start) {
        gga_start = strstr(nmea_data, "$GPGGA");
    }
    
    if(gga_start) {
        char gga_line[128] = {0};
        int i = 0;
        while(gga_start[i] && gga_start[i] != '\r' && gga_start[i] != '\n' && i < 127) {
            gga_line[i] = gga_start[i];
            i++;
        }
        gga_line[i] = '\0';
        parse_gga_sentence(gga_line);
    }
    
    // 查找GSV句子
    char *gsv_start = strstr(nmea_data, "$GPGSV");
    if(!gsv_start) {
        gsv_start = strstr(nmea_data, "$GLGSV");
    }
    
    if(gsv_start) {
        char gsv_line[256] = {0};
        int i = 0;
        while(gsv_start[i] && gsv_start[i] != '\r' && gsv_start[i] != '\n' && i < 255) {
            gsv_line[i] = gsv_start[i];
            i++;
        }
        gsv_line[i] = '\0';
        parse_gsv_sentence(gsv_line);
    }
}

static void parse_rmc_sentence(const char *rmc_line) {
    if(!rmc_line || strlen(rmc_line) < 10) {
        return;
    }
    
    char *token;
    char *line_copy = strdup(rmc_line);
    token = strtok(line_copy, ",");
    int field = 0;
    
    while(token != NULL) {
        switch(field) {
            case 0: // RMC标识符
                break;
                
            case 1: // UTC时间
                if(strlen(token) >= 6) {
                    int time = atoi(token);
                    g_gps_data.Time_H = (time / 10000) + 8; // UTC+8时区
                    g_gps_data.Time_M = (time / 100) % 100;
                    g_gps_data.Time_S = time % 100;
                    
                    if(g_gps_data.Time_H >= 24) {
                        g_gps_data.Time_H -= 24;
                    }
                }
                break;
                
            case 2: // 定位状态
                g_gps_data.Status = (token[0] == 'A') ? 1 : 0;
                break;
                
            case 3: // 纬度 (ddmm.mmmm格式)
                if(strlen(token) > 0) {
                    g_gps_data.Lat_Raw = atof(token);
                    g_gps_data.Lat = convert_nmea_to_decimal(g_gps_data.Lat_Raw);
                }
                break;
                
            case 4: // 纬度方向
                if(strlen(token) > 0) {
                    g_gps_data.Lat_area = token[0];
                    if(g_gps_data.Lat_area == 'S') {
                        g_gps_data.Lat = -g_gps_data.Lat;
                    }
                }
                break;
                
            case 5: // 经度 (dddmm.mmmm格式)
                if(strlen(token) > 0) {
                    g_gps_data.Lon_Raw = atof(token);
                    g_gps_data.Lon = convert_nmea_to_decimal(g_gps_data.Lon_Raw);
                }
                break;
                
            case 6: // 经度方向
                if(strlen(token) > 0) {
                    g_gps_data.Lon_area = token[0];
                    if(g_gps_data.Lon_area == 'W') {
                        g_gps_data.Lon = -g_gps_data.Lon;
                    }
                }
                break;
                
            case 7: // 地面速度 (节)
                if(strlen(token) > 0) {
                    g_gps_data.Speed = atof(token) * 1.852; // 转换为公里/小时
                }
                break;
                
            case 8: // 地面航向 (度)
                if(strlen(token) > 0) {
                    g_gps_data.Course = atof(token);
                }
                break;
                
            case 9: // 日期 (ddmmyy)
                if(strlen(token) >= 6) {
                    int day = (token[0] - '0') * 10 + (token[1] - '0');
                    int month = (token[2] - '0') * 10 + (token[3] - '0');
                    int year = 2000 + (token[4] - '0') * 10 + (token[5] - '0');
                    sprintf(g_gps_data.Date, "%04d-%02d-%02d", year, month, day);
                }
                break;
        }
        
        token = strtok(NULL, ",");
        field++;
    }
    
    free(line_copy);
}

static void parse_gga_sentence(const char *gga_line) {
    if(!gga_line || strlen(gga_line) < 10) {
        return;
    }
    
    char *token;
    char *line_copy = strdup(gga_line);
    token = strtok(line_copy, ",");
    int field = 0;
    
    while(token != NULL) {
        switch(field) {
            case 0: // GGA标识符
                break;
                
            case 1: // UTC时间
                if(strlen(token) >= 6) {
                    int time = atoi(token);
                    g_gps_data.Time_H = (time / 10000) + 8; // UTC+8时区
                    g_gps_data.Time_M = (time / 100) % 100;
                    g_gps_data.Time_S = time % 100;
                    
                    if(g_gps_data.Time_H >= 24) {
                        g_gps_data.Time_H -= 24;
                    }
                }
                break;
                
            case 2: // 纬度 (ddmm.mmmm格式)
                if(strlen(token) > 0) {
                    g_gps_data.Lat_Raw = atof(token);
                    g_gps_data.Lat = convert_nmea_to_decimal(g_gps_data.Lat_Raw);
                }
                break;
                
            case 3: // 纬度方向
                if(strlen(token) > 0) {
                    g_gps_data.Lat_area = token[0];
                    if(g_gps_data.Lat_area == 'S') {
                        g_gps_data.Lat = -g_gps_data.Lat;
                    }
                }
                break;
                
            case 4: // 经度 (dddmm.mmmm格式)
                if(strlen(token) > 0) {
                    g_gps_data.Lon_Raw = atof(token);
                    g_gps_data.Lon = convert_nmea_to_decimal(g_gps_data.Lon_Raw);
                }
                break;
                
            case 5: // 经度方向
                if(strlen(token) > 0) {
                    g_gps_data.Lon_area = token[0];
                    if(g_gps_data.Lon_area == 'W') {
                        g_gps_data.Lon = -g_gps_data.Lon;
                    }
                }
                break;
                
            case 6: // 定位质量指示器
                if(strlen(token) > 0 && token[0] >= '0' && token[0] <= '9') {
                    g_gps_data.Status = (token[0] == '0') ? 0 : 1;
                    g_gps_data.Quality = token[0] - '0';
                }
                break;
                
            case 7: // 使用的卫星数量
                if(strlen(token) > 0) {
                    g_gps_data.Satellites = atoi(token);
                }
                break;
                
            case 8: // HDOP
                if(strlen(token) > 0) {
                    g_gps_data.HDOP = atof(token);
                }
                break;
                
            case 9: // 海拔
                if(strlen(token) > 0) {
                    g_gps_data.Altitude = atof(token);
                }
                break;
        }
        
        token = strtok(NULL, ",");
        field++;
    }
    
    free(line_copy);
}

static void parse_gsv_sentence(const char *gsv_line) {
    if(!gsv_line || strlen(gsv_line) < 10) {
        return;
    }
    
    char *token;
    char *line_copy = strdup(gsv_line);
    token = strtok(line_copy, ",");
    int field = 0;
    uint8_t total_satellites = 0;
    uint8_t total_snr = 0;
    uint8_t snr_count = 0;
    
    while(token != NULL) {
        switch(field) {
            case 3: // 总卫星数
                total_satellites = atoi(token);
                break;
            case 7: // 第一个卫星SNR
                if(strlen(token) > 0) {
                    uint8_t snr = atoi(token);
                    if(snr > 0) {
                        total_snr += snr;
                        snr_count++;
                    }
                }
                break;
            case 11: // 第二个卫星SNR
                if(strlen(token) > 0) {
                    uint8_t snr = atoi(token);
                    if(snr > 0) {
                        total_snr += snr;
                        snr_count++;
                    }
                }
                break;
            case 15: // 第三个卫星SNR
                if(strlen(token) > 0) {
                    uint8_t snr = atoi(token);
                    if(snr > 0) {
                        total_snr += snr;
                        snr_count++;
                    }
                }
                break;
            case 19: // 第四个卫星SNR
                if(strlen(token) > 0) {
                    uint8_t snr = atoi(token);
                    if(snr > 0) {
                        total_snr += snr;
                        snr_count++;
                    }
                }
                break;
        }
        
        token = strtok(NULL, ",");
        field++;
    }
    
    // 更新卫星数据
    g_gps_data.Satellites = total_satellites;
    
    free(line_copy);
}

// =============================================================================
// 坐标转换函数实现
// =============================================================================

Coordinates lc76g_get_baidu_coordinates(void) {
    Coordinates temp;
    temp.Lat = g_gps_data.Lat;
    temp.Lon = g_gps_data.Lon;
    
    // 地图坐标系转换
    temp = transform(temp);
    temp = bd_encrypt(temp);
    return temp;
}

Coordinates lc76g_get_google_coordinates(void) {
    Coordinates temp;
    temp.Lat = g_gps_data.Lat;
    temp.Lon = g_gps_data.Lon;
    
    // 地图坐标系转换
    temp = transform(temp);
    return temp;
}
