/**
 * @file vendor_gps_parser.c
 * @brief Implementation of vendor-provided GPS parsing logic
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "gps/vendor_gps_parser.h"

// Use vendor-defined constants
#define BUFFSIZE 800

// Lookup table used by vendor code
static const char Temp[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

// Coordinate conversion constants
static const double pi = 3.14159265358979324;
static const double a = 6378245.0;
static const double ee = 0.00669342162296594323;
static const double x_pi = 3.14159265358979324 * 3000.0 / 180.0;

// GPS I2C configuration
static i2c_inst_t *gps_i2c = NULL;
static uint8_t gps_i2c_addr = 0x42;
static uint gps_sda_pin = 0;
static uint gps_scl_pin = 0;
static uint gps_i2c_speed = 100000;
static int gps_force_pin = -1;

// Data buffer
static char buff_t[BUFFSIZE] = {0};

// Global GPS data
static GNRMC GPS = {0};

// Whether to display detailed debug logs
static bool debug_output = false;

// Last valid speed and course
static double last_valid_speed = 0.0;
static double last_valid_course = 0.0;

// LC76G satellite data
static uint8_t gps_satellites_count = 0;
static uint8_t gps_signal_strength = 0;

// Forward declarations
static void parse_gsv_message(const char* gsv_data);

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
 * @brief Convert NMEA format latitude/longitude to decimal degrees format
 * 
 * Example: 3113.313760 -> 31.221896 degrees
 * 
 * @param nmea_coord Coordinate value in NMEA format
 * @return Coordinate value in decimal degrees format
 */
static double convert_nmea_to_decimal(double nmea_coord) {
    int degrees = (int)(nmea_coord / 100.0);
    double minutes = nmea_coord - (degrees * 100.0);
    return degrees + (minutes / 60.0);
}

/**
 * @brief Set whether to output detailed debug logs
 * @param enable Whether to enable
 */
void vendor_gps_set_debug(bool enable) {
    debug_output = enable;
}

/**
 * @brief Initialize the vendor version of GPS module
 */
bool vendor_gps_init(i2c_inst_t *i2c_inst, uint8_t i2c_addr, uint sda_pin, uint scl_pin, uint i2c_speed, int force_pin) {
    // Save I2C configuration
    gps_i2c = i2c_inst;
    gps_i2c_addr = i2c_addr;
    gps_sda_pin = sda_pin;
    gps_scl_pin = scl_pin;
    gps_i2c_speed = i2c_speed;
    gps_force_pin = force_pin;
    
    // Initialize I2C
    i2c_init(gps_i2c, i2c_speed);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    
    // Enable pull-ups
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    
    // Initialize control pin (if available)
    if (force_pin >= 0) {
        gpio_init(force_pin);
        gpio_set_dir(force_pin, GPIO_OUT);
        gpio_put(force_pin, 0); // Default: low level
    }
    
    printf("GPS module initialized: I2C%d, Address: 0x%02X, SDA: %d, SCL: %d, Speed: %d Hz\n", 
            i2c_inst == i2c0 ? 0 : 1, i2c_addr, sda_pin, scl_pin, i2c_speed);
    
    // Initialize GPS data
    memset(&GPS, 0, sizeof(GPS));
    
    return true;
}

/**
 * @brief Send command to GPS module
 * @param data Command string, no need to add checksum
 */
void vendor_gps_send_command(const char *data) {
    char Check = data[1], Check_char[3]={0};
    uint8_t i = 0;
    char command[256];
    int cmd_len = 0;
    
    // Calculate checksum
    for(i=2; data[i] != '\0'; i++){
        Check ^= data[i];
    }
    
    Check_char[0] = Temp[Check/16%16];
    Check_char[1] = Temp[Check%16];
    Check_char[2] = '\0';
   
    // Build complete command with checksum
    command[cmd_len++] = '\r';
    command[cmd_len++] = '\n';
    
    // Add command data
    for(i=0; data[i] != '\0' && cmd_len < 250; i++){
        command[cmd_len++] = data[i];
    }
    
    command[cmd_len++] = '*';
    command[cmd_len++] = Check_char[0];
    command[cmd_len++] = Check_char[1];
    command[cmd_len++] = '\r';
    command[cmd_len++] = '\n';
    
    // Send command via I2C
    int result = i2c_write_blocking(gps_i2c, gps_i2c_addr, (uint8_t*)command, cmd_len, false);
    
    if (debug_output) {
        if (result == PICO_ERROR_GENERIC) {
            printf("Failed to send GPS command: %s*%s\n", data, Check_char);
        } else {
            printf("Sent GPS command: %s*%s (sent %d bytes)\n", data, Check_char, result);
        }
    }
}

/**
 * @brief Make GPS module exit backup mode
 */
void vendor_gps_exit_backup_mode() {
    if (gps_force_pin >= 0) {
        gpio_put(gps_force_pin, 1);
        sleep_ms(1000);
        gpio_put(gps_force_pin, 0);
    }
}

/**
 * @brief Read a certain number of bytes from I2C
 * @param data Buffer to save data
 * @param Num Number of bytes to read
 */
static void vendor_i2c_receive_string(char *data, uint16_t Num) {
    uint16_t i = 0;
    
    // Ensure buffer is valid
    if (data == NULL || Num < 2) {
        if (debug_output) {
            printf("GPS data read error: Invalid buffer\n");
        }
        return;
    }
    
    // Clear the beginning of the buffer to ensure clean data
    memset(data, 0, Num > 10 ? 10 : Num);
    
    // Set timeout to avoid infinite waiting
    absolute_time_t timeout = make_timeout_time_ms(300);
    absolute_time_t last_read_time = get_absolute_time();
    
    // Try to read data from I2C GPS module
    // Note: LC76G GPS module may not support I2C read directly
    // This is a placeholder implementation - actual implementation depends on module capabilities
    
    while (i < Num - 1) {
        // Attempt to read from I2C
        uint8_t temp_byte = 0;
        int result = i2c_read_blocking(gps_i2c, gps_i2c_addr, &temp_byte, 1, false);
        
        if (result == PICO_OK) {
            data[i] = temp_byte;
            i++;
            last_read_time = get_absolute_time();
            
            // Check if we have a complete NMEA sentence
            if (i > 10 && (temp_byte == '\n' || temp_byte == '\r')) {
                if (debug_output) {
                    printf("GPS data read completed: Found NMEA sentence terminator\n");
                }
                break;
            }
        } else {
            // No data available, check timeout
            if (i > 10 && absolute_time_diff_us(last_read_time, get_absolute_time()) > 50000) {
                if (debug_output) {
                    printf("GPS data read completed: No new data for 50ms\n");
                }
                break;
            }
        }
        
        // Check total timeout
        if (time_reached(timeout)) {
            if (debug_output) {
                printf("GPS data read timeout, read %d bytes\n", i);
            }
            break;
        }
        
        // Short sleep to avoid busy waiting
        sleep_us(1000);
    }
    
    // Ensure data ends with NULL
    data[i] = '\0';
    
    // Check if the retrieved data is valid
    if (i < 10) {
        if (debug_output && i > 0) {
            printf("GPS data too short, possibly invalid: %s\n", data);
        }
    }
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
    char *gga_start = NULL;
    char rmc_line[128] = {0};
    bool using_gga = false;
    
    // Reset GPS status
    GPS.Status = 0;
    
    // Save previous time information to avoid flashing when resetting
    uint8_t prev_time_h = GPS.Time_H;
    uint8_t prev_time_m = GPS.Time_M;
    uint8_t prev_time_s = GPS.Time_S;
    char prev_date[11];
    strncpy(prev_date, GPS.Date, sizeof(prev_date)-1);
    prev_date[sizeof(prev_date)-1] = '\0';
    
    // Save previous location and speed information
    double prev_lat = GPS.Lat;
    double prev_lon = GPS.Lon;
    double prev_speed = GPS.Speed;
    double prev_course = GPS.Course;
    double prev_altitude = GPS.Altitude;
    
    // Temporarily reset other values except time, waiting for new data to update
    GPS.Lat = 0.0;
    GPS.Lon = 0.0;
    GPS.Speed = prev_speed;  // Keep previous speed to avoid flashing speed display
    GPS.Course = prev_course;  // Keep previous course to avoid flashing course
    GPS.Altitude = prev_altitude;  // Keep previous altitude value
    
    // Use vendor's original code to receive NMEA data stream
    vendor_i2c_receive_string(buff_t, BUFFSIZE);
    
    // Print original RAW data before any processing
    if (debug_output) {
        printf("[GPS原始数据] 接收到的原始数据 (长度: %d):\n", (int)strlen(buff_t));
        printf("--- ORIGINAL RAW DATA START ---\n");
        printf("%s", buff_t);
        printf("\n--- ORIGINAL RAW DATA END ---\n");
    }
    
    // Clean up corrupted data - keep all NMEA standard characters
    char cleaned_buffer[BUFFSIZE] = {0};
    int clean_index = 0;
    for (int i = 0; i < BUFFSIZE && buff_t[i] != '\0'; i++) {
        char c = buff_t[i];
        // Keep all NMEA standard characters: $, *, comma, digits, letters, CR, LF
        // This includes all printable characters plus NMEA control characters
        if (c >= 32 && c <= 126) {
            cleaned_buffer[clean_index++] = c;
        } else if (c == '\r' || c == '\n' || c == '\t') {
            cleaned_buffer[clean_index++] = c;
        }
        // Skip only truly problematic characters (null, control chars except CR/LF)
    }
    cleaned_buffer[clean_index] = '\0';
    
    // Print cleaned data after character filtering
    if (debug_output) {
        printf("[GPS清理数据] 字符过滤后的数据 (长度: %d):\n", (int)strlen(cleaned_buffer));
        printf("--- CLEANED DATA START ---\n");
        printf("%s", cleaned_buffer);
        printf("\n--- CLEANED DATA END ---\n");
    }
    
    // NMEA standard compliant cleanup: preserve all valid NMEA data
    // Only remove clearly corrupted data, keep ERROR strings as they might be valid GPS output
    char* src = cleaned_buffer;
    char* dst = buff_t;
    
    while (*src && (dst - buff_t) < BUFFSIZE - 1) {
        // Keep all data that could be part of NMEA sentences
        // ERROR strings are kept as they might be valid GPS module output
        *dst++ = *src++;
    }
    *dst = '\0';
    
    // Print final processed data
    if (debug_output) {
        printf("[GPS最终数据] 垃圾清理后的最终数据 (长度: %d):\n", (int)strlen(buff_t));
        printf("--- FINAL DATA START ---\n");
        printf("%s", buff_t);
        printf("\n--- FINAL DATA END ---\n");
    }
    
    // If debugging, print received data
    if (debug_output) {
        // Print first 100 characters for quick overview
        char preview[101] = {0};
        strncpy(preview, buff_t, 100);
        printf("First 100 characters of GPS data: %s\n", preview);
        
        // Print complete RAW data for detailed analysis
        printf("[GPS RAW数据] 完整数据流 (长度: %d):\n", (int)strlen(buff_t));
        printf("--- RAW DATA START ---\n");
        printf("%s", buff_t);
        printf("\n--- RAW DATA END ---\n");
        
        // Additional debug: check for NMEA sentence patterns
        if (strstr(buff_t, "$GNGGA") || strstr(buff_t, "$GPRMC") || strstr(buff_t, "$GNRMC")) {
            printf("[GPS调试] 检测到NMEA句子模式\n");
        } else {
            printf("[GPS调试] 未检测到NMEA句子模式\n");
        }
        
        // Count different sentence types with checksum validation
        int gga_count = 0, rmc_count = 0, gsv_count = 0, gsa_count = 0;
        int valid_checksum_count = 0, invalid_checksum_count = 0;
        char* ptr = buff_t;
        
        // Count sentences and validate checksums
        while ((ptr = strstr(ptr, "$")) != NULL) {
            char* end_ptr = strchr(ptr, '\n');
            if (!end_ptr) end_ptr = strchr(ptr, '\r');
            if (!end_ptr) end_ptr = ptr + strlen(ptr);
            
            // Check if this looks like a complete NMEA sentence
            char* checksum_pos = strchr(ptr, '*');
            if (checksum_pos && checksum_pos < end_ptr) {
                // Validate checksum
                uint8_t calculated_checksum = 0;
                for (char* p = ptr + 1; p < checksum_pos; p++) {
                    calculated_checksum ^= *p;
                }
                
                // Parse provided checksum
                uint8_t provided_checksum = 0;
                if (sscanf(checksum_pos + 1, "%2hhx", &provided_checksum) == 1) {
                    if (calculated_checksum == provided_checksum) {
                        valid_checksum_count++;
                    } else {
                        invalid_checksum_count++;
                    }
                }
            }
            
            // Count sentence types
            if (strncmp(ptr, "$GNGGA", 6) == 0 || strncmp(ptr, "$GPGGA", 6) == 0) gga_count++;
            else if (strncmp(ptr, "$GNRMC", 6) == 0 || strncmp(ptr, "$GPRMC", 6) == 0) rmc_count++;
            else if (strncmp(ptr, "$GPGSV", 6) == 0 || strncmp(ptr, "$GLGSV", 6) == 0) gsv_count++;
            else if (strncmp(ptr, "$GNGSA", 6) == 0 || strncmp(ptr, "$GPGSA", 6) == 0) gsa_count++;
            
            ptr++;
        }
        
        printf("[GPS统计] GGA: %d, RMC: %d, GSV: %d, GSA: %d\n", gga_count, rmc_count, gsv_count, gsa_count);
        printf("[GPS校验] 有效校验和: %d, 无效校验和: %d\n", valid_checksum_count, invalid_checksum_count);
    }
    
    // Parse GSV messages for satellite information
    char* gsv_start = strstr(buff_t, "$GPGSV");
    if (!gsv_start) {
        gsv_start = strstr(buff_t, "$GLGSV");
    }
    if (!gsv_start) {
        gsv_start = strstr(buff_t, "$GAGSV");
    }
    if (!gsv_start) {
        gsv_start = strstr(buff_t, "$GBGSV");
    }
    if (!gsv_start) {
        gsv_start = strstr(buff_t, "$GQGSV");
    }
    
    if (gsv_start) {
        // Extract complete GSV sentence
        char gsv_line[256] = {0};
        int i = 0;
        while (gsv_start[i] && gsv_start[i] != '\r' && gsv_start[i] != '\n' && i < 255) {
            gsv_line[i] = gsv_start[i];
            i++;
        }
        gsv_line[i] = '\0';
        
        // Validate GSV sentence format - be more lenient for partial sentences
        if (strlen(gsv_line) > 10) {
            if (debug_output) {
                printf("Parsing GSV sentence: %s\n", gsv_line);
            }
            parse_gsv_message(gsv_line);
        } else if (debug_output) {
            printf("Invalid GSV sentence format: %s\n", gsv_line);
        }
    }
    
    // Find GNRMC or GPRMC sentence
    rmc_start = strstr(buff_t, "$GNRMC");
    if (!rmc_start) {
        rmc_start = strstr(buff_t, "$GPRMC"); // Try to find GPRMC
    }
    
    // If no RMC sentence is found, try GGA sentence
    if (!rmc_start) {
        gga_start = strstr(buff_t, "$GNGGA");
        if (!gga_start) {
            gga_start = strstr(buff_t, "$GPGGA"); // Try to find GPGGA
        }
        
        if (gga_start) {
            rmc_start = gga_start;
            using_gga = true;
        } else {
            if (debug_output) {
                printf("No RMC or GGA sentence found\n");
            }
            
            // If no valid sentence is found, keep previous time information to avoid flashing
            GPS.Time_H = prev_time_h;
            GPS.Time_M = prev_time_m;
            GPS.Time_S = prev_time_s;
            strncpy(GPS.Date, prev_date, sizeof(GPS.Date)-1);
            GPS.Date[sizeof(GPS.Date)-1] = '\0';
            
            return GPS; // No valid sentence found, return previous data
        }
    }
    
    // Extract complete sentence
    int i = 0;
    while (rmc_start[i] && rmc_start[i] != '\r' && rmc_start[i] != '\n' && i < 127) {
        rmc_line[i] = rmc_start[i];
        i++;
    }
    rmc_line[i] = '\0';
    
    // Validate sentence format
    if (strlen(rmc_line) < 10 || strchr(rmc_line, '*') == NULL) {
        if (debug_output) {
            printf("Invalid %s sentence format: %s\n", using_gga ? "GGA" : "RMC", rmc_line);
        }
        
        // If no valid sentence is found, keep previous time information to avoid flashing
        GPS.Time_H = prev_time_h;
        GPS.Time_M = prev_time_m;
        GPS.Time_S = prev_time_s;
        strncpy(GPS.Date, prev_date, sizeof(GPS.Date)-1);
        GPS.Date[sizeof(GPS.Date)-1] = '\0';
        
        return GPS; // Invalid sentence format, return previous data
    }
    
    if (debug_output) {
        if (using_gga) {
            printf("Parsing GGA sentence: %s\n", rmc_line);
        } else {
            printf("Parsing RMC sentence: %s\n", rmc_line);
        }
    }
    
    // Use strtok to parse the various fields of the NMEA sentence
    token = strtok(rmc_line, ",");
    int field = 0;
    
    if (using_gga) {
        // Parse GGA sentence
        // $GNGGA,hhmmss.sss,ddmm.mmmm,N,dddmm.mmmm,E,q,ss,y.y,a.a,M,b.b,M,c.c,rrrr*hh
        // Where:
        // - hhmmss.sss  is UTC time
        // - ddmm.mmmm  is latitude, format ddmm.mmmm (degree minute format)
        // - N  is latitude direction, N=North, S=South
        // - dddmm.mmmm  is longitude, format dddmm.mmmm (degree minute format)
        // - E  is longitude direction, E=East, W=West
        // - q  is positioning quality indicator (0=invalid, 1=GPS positioning, 2=DGPS positioning)
        // - ss  is number of satellites being used
        // - y.y  is HDOP horizontal precision factor
        // - a.a  is altitude
        // - M  is altitude unit, M represents meters
        // - b.b  is geoid separation
        // - M  is geoid separation unit, M represents meters
        // - c.c  is DGPS data age
        // - rrrr  is DGPS reference station ID
        
        while (token != NULL) {
            switch (field) {
                case 0: // GGA identifier
                    break;
                    
                case 1: // UTC time
                    if (strlen(token) >= 6) {
                        // Extract time directly from string
                        int time = atoi(token);
                        GPS.Time_H = (time / 10000) + 8; // UTC+8 timezone
                        GPS.Time_M = (time / 100) % 100;
                        GPS.Time_S = time % 100;
                        
                        // Handle cross-day situation
                        if (GPS.Time_H >= 24) {
                            GPS.Time_H -= 24;
                        }
                    }
                    break;
                    
                case 2: // Latitude (ddmm.mmmm format)
                    if (strlen(token) > 0) {
                        // Save original format for reference
                        GPS.Lat_Raw = atof(token);
                        // Convert to decimal degree format
                        GPS.Lat = convert_nmea_to_decimal(GPS.Lat_Raw);
                    }
                    break;
                    
                case 3: // Latitude direction
                    if (strlen(token) > 0) {
                        GPS.Lat_area = token[0];
                        // If it's south latitude, latitude is negative
                        if (GPS.Lat_area == 'S') {
                            GPS.Lat = -GPS.Lat;
                        }
                    }
                    break;
                    
                case 4: // Longitude (dddmm.mmmm format)
                    if (strlen(token) > 0) {
                        // Save original format for reference
                        GPS.Lon_Raw = atof(token);
                        // Convert to decimal degree format
                        GPS.Lon = convert_nmea_to_decimal(GPS.Lon_Raw);
                    }
                    break;
                    
                case 5: // Longitude direction
                    if (strlen(token) > 0) {
                        GPS.Lon_area = token[0];
                        // If it's west longitude, longitude is negative
                        if (GPS.Lon_area == 'W') {
                            GPS.Lon = -GPS.Lon;
                        }
                    }
                    break;
                    
                case 6: // Positioning quality indicator
                    if (strlen(token) > 0 && token[0] >= '0' && token[0] <= '9') {
                        GPS.Status = (token[0] == '0') ? 0 : 1;
                        if (debug_output) {
                            printf("GGA positioning quality: %c -> Status=%d\n", token[0], GPS.Status);
                        }
                    } else {
                        // Invalid or empty quality indicator
                        GPS.Status = 0;
                        if (debug_output) {
                            printf("GGA positioning quality: invalid/empty -> Status=0\n");
                        }
                    }
                    break;
                    
                case 9: // Altitude
                    if (strlen(token) > 0) {
                        GPS.Altitude = atof(token);
                        if (debug_output) {
                            printf("Extracted altitude: %.3f meters\n", GPS.Altitude);
                        }
                    }
                    break;
            }
            
            token = strtok(NULL, ",");
            field++;
        }
        
        // Use previously saved speed and course information because GGA sentence does not contain these
        GPS.Speed = prev_speed;
        GPS.Course = prev_course;
    } else {
        // Original RMC parsing logic
        while (token != NULL) {
            switch (field) {
                case 0: // RMC identifier
                    break;
                    
                case 1: // UTC time
                    if (strlen(token) >= 6) {
                        // Extract time directly from string
                        int time = atoi(token);
                        GPS.Time_H = (time / 10000) + 8; // UTC+8 timezone
                        GPS.Time_M = (time / 100) % 100;
                        GPS.Time_S = time % 100;
                        
                        // Handle cross-day situation
                        if (GPS.Time_H >= 24) {
                            GPS.Time_H -= 24;
                        }
                    }
                    break;
                    
                case 2: // Positioning status
                    GPS.Status = (token[0] == 'A') ? 1 : 0;
                    break;
                    
                case 3: // Latitude (ddmm.mmmm format)
                    if (strlen(token) > 0) {
                        // Save original format for reference
                        GPS.Lat_Raw = atof(token);
                        // Convert to decimal degree format
                        GPS.Lat = convert_nmea_to_decimal(GPS.Lat_Raw);
                    }
                    break;
                    
                case 4: // Latitude direction
                    if (strlen(token) > 0) {
                        GPS.Lat_area = token[0];
                        // If it's south latitude, latitude is negative
                        if (GPS.Lat_area == 'S') {
                            GPS.Lat = -GPS.Lat;
                        }
                    }
                    break;
                    
                case 5: // Longitude (dddmm.mmmm format)
                    if (strlen(token) > 0) {
                        // Save original format for reference
                        GPS.Lon_Raw = atof(token);
                        // Convert to decimal degree format
                        GPS.Lon = convert_nmea_to_decimal(GPS.Lon_Raw);
                    }
                    break;
                    
                case 6: // Longitude direction
                    if (strlen(token) > 0) {
                        GPS.Lon_area = token[0];
                        // If it's west longitude, longitude is negative
                        if (GPS.Lon_area == 'W') {
                            GPS.Lon = -GPS.Lon;
                        }
                    }
                    break;
                    
                case 7: // Ground speed (knots)
                    if (strlen(token) > 0) {
                        GPS.Speed = atof(token) * 1.852; // Convert to kilometers/hour
                        // Save valid speed value for subsequent use
                        last_valid_speed = GPS.Speed;
                    }
                    break;
                    
                case 8: // Ground course (degrees)
                    if (strlen(token) > 0) {
                        GPS.Course = atof(token);
                        // Save valid course value for subsequent use
                        last_valid_course = GPS.Course;
                    }
                    break;
                    
                case 9: // Date (ddmmyy)
                    if (strlen(token) >= 6) {
                        // Save original date, format DDMMYY
                        int day = (token[0] - '0') * 10 + (token[1] - '0');
                        int month = (token[2] - '0') * 10 + (token[3] - '0');
                        int year = 2000 + (token[4] - '0') * 10 + (token[5] - '0');
                        
                        // Format date string
                        sprintf(GPS.Date, "%04d-%02d-%02d", year, month, day);
                    }
                    break;
            }
            
            token = strtok(NULL, ",");
            field++;
        }
    }
    
    if (debug_output && GPS.Status) {
        printf("GPS positioning successful: Latitude=%.6f%c(%.6f°), Longitude=%.6f%c(%.6f°)\n", 
              GPS.Lat_Raw, GPS.Lat_area, GPS.Lat,
              GPS.Lon_Raw, GPS.Lon_area, GPS.Lon);
    }
    
    // Output additional debug information
    if (debug_output) {
        printf("GPS data status: Positioning status=%d, Latitude=%.6f, Longitude=%.6f, Data type=%s\n", 
               GPS.Status, GPS.Lat, GPS.Lon, using_gga ? "GGA" : "RMC");
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
    
    // Directly use converted decimal degree coordinate
    temp.Lat = GPS.Lat;
    temp.Lon = GPS.Lon;
    
    // Map coordinate system conversion
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
    
    // Directly use converted decimal degree coordinate
    temp.Lat = GPS.Lat;
    temp.Lon = GPS.Lon;
    
    // Map coordinate system conversion
    temp = transform(temp);
    return temp;
}

// =============================================================================
// LC76G Enhanced Functions
// =============================================================================

/**
 * @brief Calculate NMEA checksum
 * @param data String data (without $ and *checksum)
 * @return Calculated checksum
 */
static uint8_t calculate_nmea_checksum(const char* data) {
    uint8_t checksum = 0;
    for (int i = 0; data[i] != '\0'; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief Send PAIR command to LC76G module
 * @param command_id Command ID (e.g., 050, 062, 864)
 * @param params Command parameters (comma-separated)
 * @return PAIR response structure
 */
PAIRResponse vendor_gps_send_pair_command(uint16_t command_id, const char* params) {
    PAIRResponse response = {0};
    char command[128];
    char response_buffer[256];
    uint8_t checksum;
    
    // Build PAIR command
    if (params && strlen(params) > 0) {
        snprintf(command, sizeof(command), "$PAIR%03d,%s", command_id, params);
    } else {
        snprintf(command, sizeof(command), "$PAIR%03d", command_id);
    }
    
    // Calculate checksum
    checksum = calculate_nmea_checksum(command + 1); // Skip '$'
    snprintf(command + strlen(command), sizeof(command) - strlen(command), "*%02X\r\n", checksum);
    
    if (debug_output) {
        printf("Sending PAIR command: %s", command);
    }
    
    // Send command via I2C
    int result = i2c_write_blocking(gps_i2c, gps_i2c_addr, (uint8_t*)command, strlen(command), false);
    if (result == PICO_ERROR_GENERIC) {
        if (debug_output) {
            printf("Failed to send PAIR command\n");
        }
        return response;
    }
    
    // Wait for response (with timeout)
    absolute_time_t timeout = make_timeout_time_ms(1000);
    uint16_t response_len = 0;
    
    while (!time_reached(timeout) && response_len < sizeof(response_buffer) - 1) {
        uint8_t temp_byte = 0;
        int result = i2c_read_blocking(gps_i2c, gps_i2c_addr, &temp_byte, 1, false);
        
        if (result == PICO_OK) {
            response_buffer[response_len++] = temp_byte;
            
            // Check if we have a complete response
            if (temp_byte == '\n' && response_len > 10) {
                response_buffer[response_len] = '\0';
                break;
            }
        } else {
            // No data available, wait a bit
            sleep_us(1000);
        }
    }
    
    if (response_len > 0) {
        response_buffer[response_len] = '\0';
        
        // Parse PAIR response: $PAIR001,<CommandID>,<Result>*<Checksum>
        char* pair_start = strstr(response_buffer, "$PAIR001");
        if (pair_start) {
            // Find the end of the response (before checksum)
            char* checksum_start = strchr(pair_start, '*');
            if (checksum_start) {
                *checksum_start = '\0'; // Null terminate before checksum
            }
            
            char* token = strtok(pair_start, ",");
            int field = 0;
            
            while (token != NULL && field < 3) {
                switch (field) {
                    case 1: // Command ID
                        if (strlen(token) > 0) {
                            response.CommandID = atoi(token);
                        }
                        break;
                    case 2: // Result
                        if (strlen(token) > 0) {
                            response.Result = atoi(token);
                        }
                        break;
                }
                token = strtok(NULL, ",");
                field++;
            }
            
            response.Valid = true;
            
            if (debug_output) {
                printf("PAIR response: CommandID=%d, Result=%d\n", 
                       response.CommandID, response.Result);
            }
        }
    }
    
    return response;
}

/**
 * @brief Set LC76G positioning rate
 * @param rate_ms Rate in milliseconds (100-1000)
 * @return Whether command was successful
 */
bool vendor_gps_set_positioning_rate(uint16_t rate_ms) {
    if (rate_ms < 100 || rate_ms > 1000) {
        return false;
    }
    
    char params[16];
    snprintf(params, sizeof(params), "%d", rate_ms);
    
    PAIRResponse response = vendor_gps_send_pair_command(050, params);
    return response.Valid && response.Result == 0;
}

/**
 * @brief Set LC76G NMEA message output rate
 * @param message_type Message type (0=GGA, 1=GLL, 2=GSA, 3=GSV, 4=RMC, 5=VTG)
 * @param output_rate Output rate (0=disable, 1-20=every N fixes)
 * @return Whether command was successful
 */
bool vendor_gps_set_nmea_output_rate(uint8_t message_type, uint8_t output_rate) {
    if (message_type > 5 || output_rate > 20) {
        return false;
    }
    
    char params[16];
    snprintf(params, sizeof(params), "%d,%d", message_type, output_rate);
    
    PAIRResponse response = vendor_gps_send_pair_command(062, params);
    return response.Valid && response.Result == 0;
}

/**
 * @brief Set LC76G baud rate
 * @param baud_rate Baud rate (9600, 115200, 230400, 460800, 921600, 3000000)
 * @return Whether command was successful
 */
bool vendor_gps_set_baud_rate(uint32_t baud_rate) {
    uint32_t valid_rates[] = {9600, 115200, 230400, 460800, 921600, 3000000};
    bool valid = false;
    
    for (int i = 0; i < 6; i++) {
        if (baud_rate == valid_rates[i]) {
            valid = true;
            break;
        }
    }
    
    if (!valid) {
        return false;
    }
    
    char params[32];
    snprintf(params, sizeof(params), "0,0,%d", baud_rate);
    
    PAIRResponse response = vendor_gps_send_pair_command(864, params);
    return response.Valid && response.Result == 0;
}

/**
 * @brief Perform LC76G cold start
 * @return Whether command was successful
 */
bool vendor_gps_cold_start(void) {
    PAIRResponse response = vendor_gps_send_pair_command(007, NULL);
    return response.Valid && response.Result == 0;
}

/**
 * @brief Perform LC76G hot start
 * @return Whether command was successful
 */
bool vendor_gps_hot_start(void) {
    PAIRResponse response = vendor_gps_send_pair_command(004, NULL);
    return response.Valid && response.Result == 0;
}

/**
 * @brief Perform LC76G warm start
 * @return Whether command was successful
 */
bool vendor_gps_warm_start(void) {
    PAIRResponse response = vendor_gps_send_pair_command(005, NULL);
    return response.Valid && response.Result == 0;
}

/**
 * @brief Read RTC time from LC76G module
 * @param rtc_time Pointer to store RTC time (seconds since epoch)
 * @return Whether command was successful
 */
bool vendor_gps_read_rtc_time(uint32_t* rtc_time) {
    if (!rtc_time) return false;
    
    // 注意：LC76G可能不支持PAIR008命令
    // 这里先返回失败，使用备用方案
    printf("[GPS启动] 警告：LC76G可能不支持RTC时间读取命令\n");
    return false;
    
    // 如果LC76G支持RTC读取，需要根据实际响应格式解析
    // PAIRResponse response = vendor_gps_send_pair_command(8, NULL);
    // if (response.Valid && response.Result == 0) {
    //     // 需要解析响应中的时间数据，不是response.Result
    //     *rtc_time = parse_rtc_time_from_response(response);
    //     return true;
    // }
    // return false;
}

/**
 * @brief Set RTC time in LC76G module
 * @param rtc_time RTC time (seconds since epoch)
 * @return Whether command was successful
 */
bool vendor_gps_set_rtc_time(uint32_t rtc_time) {
    char params[16];
    snprintf(params, sizeof(params), "%u", rtc_time);
    
    PAIRResponse response = vendor_gps_send_pair_command(9, params);
    return response.Valid && response.Result == 0;
}

/**
 * @brief Smart GPS start based on power-off duration
 * @param power_off_duration Duration in seconds since last GPS fix
 * @return Whether start command was successful
 */
bool vendor_gps_smart_start(uint32_t power_off_duration) {
    printf("[GPS启动] 断电时长: %u秒 (%.1f小时)\n", 
           power_off_duration, power_off_duration / 3600.0f);
    
    if (power_off_duration < 7200) {      // 2小时内 - 热启动
        printf("[GPS启动] 执行热启动 (TTFF: 1-10秒)\n");
        return vendor_gps_hot_start();
    } else if (power_off_duration < 86400) { // 24小时内 - 温启动
        printf("[GPS启动] 执行温启动 (TTFF: 20-40秒)\n");
        return vendor_gps_warm_start();
    } else {                              // 超过24小时 - 冷启动
        printf("[GPS启动] 执行冷启动 (TTFF: 30秒以上)\n");
        return vendor_gps_cold_start();
    }
}

/**
 * @brief Save LC76G configuration to flash
 * @return Whether command was successful
 */
bool vendor_gps_save_config(void) {
    PAIRResponse response = vendor_gps_send_pair_command(513, NULL);
    return response.Valid && response.Result == 0;
}

/**
 * @brief Set LC76G satellite systems
 * @param gps Enable GPS (1=enabled, 0=disabled)
 * @param glonass Enable GLONASS (1=enabled, 0=disabled)
 * @param galileo Enable Galileo (1=enabled, 0=disabled)
 * @param bds Enable BDS (1=enabled, 0=disabled)
 * @param qzss Enable QZSS (1=enabled, 0=disabled)
 * @return Whether command was successful
 */
bool vendor_gps_set_satellite_systems(uint8_t gps, uint8_t glonass, uint8_t galileo, uint8_t bds, uint8_t qzss) {
    char params[32];
    snprintf(params, sizeof(params), "%d,%d,%d,%d,%d,0", gps, glonass, galileo, bds, qzss);
    
    PAIRResponse response = vendor_gps_send_pair_command(066, params);
    return response.Valid && response.Result == 0;
}

/**
 * @brief Parse GSV message to extract satellite information
 * @param gsv_data GSV message data
 */
static void parse_gsv_message(const char* gsv_data) {
    char* token;
    char* gsv_copy = strdup(gsv_data);
    int field = 0;
    uint8_t total_satellites = 0;
    uint8_t satellites_in_message = 0;
    uint8_t total_snr = 0;
    uint8_t snr_count = 0;
    
    token = strtok(gsv_copy, ",");
    
    while (token != NULL) {
        switch (field) {
            case 3: // Total number of satellites
                total_satellites = atoi(token);
                break;
            case 7: // First satellite SNR
                if (strlen(token) > 0) {
                    uint8_t snr = atoi(token);
                    if (snr > 0) {
                        total_snr += snr;
                        snr_count++;
                    }
                }
                break;
            case 11: // Second satellite SNR
                if (strlen(token) > 0) {
                    uint8_t snr = atoi(token);
                    if (snr > 0) {
                        total_snr += snr;
                        snr_count++;
                    }
                }
                break;
            case 15: // Third satellite SNR
                if (strlen(token) > 0) {
                    uint8_t snr = atoi(token);
                    if (snr > 0) {
                        total_snr += snr;
                        snr_count++;
                    }
                }
                break;
            case 19: // Fourth satellite SNR
                if (strlen(token) > 0) {
                    uint8_t snr = atoi(token);
                    if (snr > 0) {
                        total_snr += snr;
                        snr_count++;
                    }
                }
                break;
        }
        
        token = strtok(NULL, ",");
        field++;
    }
    
    // Update global satellite data
    gps_satellites_count = total_satellites;
    
    if (snr_count > 0) {
        uint8_t avg_snr = total_snr / snr_count;
        // Convert SNR (0-99 dB-Hz) to signal strength (0-100)
        gps_signal_strength = (avg_snr * 100) / 99;
        if (gps_signal_strength > 100) gps_signal_strength = 100;
    }
    
    free(gsv_copy);
}

/**
 * @brief Get LC76G satellite count from GSV messages
 * @return Number of satellites in view
 */
uint8_t vendor_gps_get_satellite_count(void) {
    return gps_satellites_count;
}

/**
 * @brief Get LC76G signal strength from GSV messages
 * @return Average signal strength (0-100)
 */
uint8_t vendor_gps_get_signal_strength(void) {
    return gps_signal_strength;
} 