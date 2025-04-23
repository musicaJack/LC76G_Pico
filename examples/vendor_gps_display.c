/**
 * @file vendor_gps_display.c
 * @brief GPS coordinates reception and display on LCD - Integrated with vendor code
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "display/st7789.h"
#include "display/st7789_gfx.h"
#include "gps/vendor_gps_parser.h"

// Screen parameter definitions
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// GPS UART configuration
#define GPS_UART_ID    0          // Use UART0
#define GPS_TX_PIN     0          // GPIO0 (UART0 TX)
#define GPS_RX_PIN     1          // GPIO1 (UART0 RX)
#define GPS_FORCE_PIN  4          // GPIO4 (FORCE pin)
#define GPS_BAUD_RATE  115200     // GPS module baud rate set to 115200

// GPS data validation settings
#define GPS_VALID_COORD_THRESHOLD 1.0  // Valid GPS coordinates must be greater than this value

// Color definitions - Extended color scheme besides standard colors
#define COLOR_BACKGROUND   0x0841  // Deep blue background
#define COLOR_TITLE        0xFFFF  // White title
#define COLOR_LABEL        0xAD55  // Light gray label
#define COLOR_VALUE        0x07FF  // Cyan value
#define COLOR_WARNING      0xF800  // Red warning
#define COLOR_GOOD         0x07E0  // Green normal status
#define COLOR_BORDER       0x6B5A  // Border color
#define COLOR_GRID         0x39C7  // Grid line color
#define COLOR_BAIDU        0xFD20  // Baidu map data special color (orange)

// Font size definitions
#define FONT_SIZE_TITLE    1       // Title font size
#define FONT_SIZE_LABEL    1       // Label font size
#define FONT_SIZE_VALUE    1       // Value font size

// Debug settings
static bool enable_debug = true;  // Whether to display detailed debug information

// GPS data structure - Extended version
typedef struct {
    // Original GPS data
    float latitude;      // Latitude (decimal degree format)
    float longitude;     // Longitude (decimal degree format)
    float speed;         // Ground speed (km/h)
    float course;        // Course angle (degrees)
    bool fix;            // Positioning status
    char timestamp[9];   // Timestamp "HH:MM:SS"
    char datestamp[11];  // Date "YYYY-MM-DD"
    
    // Baidu coordinates data
    float baidu_lat;     // Baidu map latitude
    float baidu_lon;     // Baidu map longitude
    
    // Google coordinates data
    float google_lat;    // Google map latitude
    float google_lon;    // Google map longitude
    
    // Status indicators
    int satellites;      // Number of satellites (virtual value, L76X does not provide this information)
    float hdop;          // Horizontal dilution of precision (virtual value, L76X does not provide this information)
    float altitude;      // Altitude information (meters)
} extended_gps_data_t;

// Global GPS data
static extended_gps_data_t gps_data = {0};
static uint32_t packet_count = 0;
static uint32_t valid_fix_count = 0;  // Valid fix count

// Function forward declarations
static void draw_gps_ui_frame(void);
static void draw_satellite_signal(int satellites);
static void draw_empty_satellite_signal(void);
static void update_gps_display(void);
static bool update_gps_data_from_module(void);
static void print_gps_debug_info(bool force_print);

/**
 * @brief Draw the basic framework of GPS UI interface
 */
static void draw_gps_ui_frame(void) {
    // Clear screen with background color
    st7789_fill_screen(COLOR_BACKGROUND);
    
    // Draw title area
    st7789_fill_rect(0, 0, SCREEN_WIDTH, 30, ST7789_BLUE);
    st7789_draw_string(50, 8, "L76X GPS Monitor", COLOR_TITLE, ST7789_BLUE, FONT_SIZE_TITLE);
    
    // Draw outer border
    st7789_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BORDER);
    
    // Draw separator lines
    st7789_draw_hline(0, 30, SCREEN_WIDTH, COLOR_BORDER);
    st7789_draw_hline(0, 220, SCREEN_WIDTH, COLOR_BORDER);
    
    // Draw satellite signal strength area title
    st7789_draw_string(20, 226, "Satellite Signal", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    
    // Draw fixed labels
    st7789_draw_string(10, 50, "Baidu Lat:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 75, "Baidu Lon:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 100, "Speed:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 125, "Course:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 150, "Date:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
    st7789_draw_string(10, 175, "Time:", COLOR_LABEL, COLOR_BACKGROUND, FONT_SIZE_LABEL);
}

/**
 * @brief Draw satellite signal strength graph (simulated value, L76X does not provide this information)
 * @param satellites Number of satellites
 */
static void draw_satellite_signal(int satellites) {
    // Clear signal area
    st7789_fill_rect(10, 255, SCREEN_WIDTH - 20, 55, COLOR_BACKGROUND);
    
    // Draw signal strength bars
    int max_satellites = 8; // Reduce maximum display count to make each signal box larger
    int bar_width = (SCREEN_WIDTH - 40) / max_satellites;
    
    for (int i = 0; i < max_satellites; i++) {
        int x = 20 + i * bar_width;
        int max_height = 45; // Increase height
        int height;
        
        if (i < satellites && satellites <= max_satellites) {
            // Randomly simulate different signal strengths (1-5)
            int strength = rand() % 5 + 1;
            height = strength * max_height / 5;
            
            // Choose color based on strength
            uint16_t color;
            if (strength < 2) color = ST7789_RED;
            else if (strength < 4) color = ST7789_YELLOW;
            else color = ST7789_GREEN;
            
            st7789_fill_rect(x, 300 - height, bar_width - 4, height, color);
        } else {
            // Unused satellite positions displayed as gray empty bars
            st7789_draw_rect(x, 255, bar_width - 4, max_height, COLOR_GRID);
        }
    }
}

/**
 * @brief Draw empty satellite signal boxes (used when not positioning)
 */
static void draw_empty_satellite_signal(void) {
    // Clear signal area
    st7789_fill_rect(10, 255, SCREEN_WIDTH - 20, 55, COLOR_BACKGROUND);
    
    // Draw empty signal boxes
    int max_satellites = 8; // Reduce maximum display count to make each signal box larger
    int bar_width = (SCREEN_WIDTH - 40) / max_satellites;
    
    for (int i = 0; i < max_satellites; i++) {
        int x = 20 + i * bar_width;
        int max_height = 45; // Increase height
        
        // Only draw empty signal box borders
        st7789_draw_rect(x, 255, bar_width - 4, max_height, COLOR_GRID);
    }
}

/**
 * @brief Update GPS data display
 */
static void update_gps_display(void) {
    char buffer[64]; // Character buffer
    static char prev_lat[20] = {0};
    static char prev_lon[20] = {0};
    static char prev_speed[20] = {0};
    static char prev_course[20] = {0};
    static char prev_date[20] = {0};
    static char prev_time[20] = {0};
    static bool prev_fix_state = false;
    
    // Prepare new data strings
    char new_lat[20], new_lon[20], new_speed[20], new_course[20];
    
    // Ensure signal boxes are displayed regardless of positioning status
    if (packet_count <= 1 || (prev_fix_state != gps_data.fix && !gps_data.fix)) {
        // First display or changed from positioning to no positioning, draw empty signal boxes
        draw_empty_satellite_signal();
    } else if (prev_fix_state != gps_data.fix && gps_data.fix) {
        // Changed from no positioning to positioning, draw signal strength
        draw_satellite_signal(gps_data.satellites);
    }
    
    // Update satellite signal display every 10 data packets to prevent flickering too quickly
    if (packet_count % 10 == 0) {
        if (gps_data.fix) {
            draw_satellite_signal(gps_data.satellites);
        } else {
            draw_empty_satellite_signal();
        }
    }
    
    if (gps_data.fix) {
        // Prepare new data strings
        snprintf(new_lat, sizeof(new_lat), "%.6f", gps_data.baidu_lat);
        snprintf(new_lon, sizeof(new_lon), "%.6f", gps_data.baidu_lon);
        snprintf(new_speed, sizeof(new_speed), "%.1f km/h", gps_data.speed);
        snprintf(new_course, sizeof(new_course), "%.1f\xF8", gps_data.course);
        
        // Color settings: Use normal color when positioning is successful
        uint16_t lat_lon_color = COLOR_BAIDU;
        uint16_t value_color = COLOR_VALUE;
        
        // Update display only when value changes or status changes
        if (strcmp(new_lat, prev_lat) != 0 || prev_fix_state != gps_data.fix) {
            // Erase old value with background color
            st7789_fill_rect(100, 50, 130, 18, COLOR_BACKGROUND);
            // Display new value
            st7789_draw_string(100, 50, new_lat, lat_lon_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_lat, new_lat);
        }
        
        if (strcmp(new_lon, prev_lon) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 75, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 75, new_lon, lat_lon_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_lon, new_lon);
        }
        
        if (strcmp(new_speed, prev_speed) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 100, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 100, new_speed, value_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_speed, new_speed);
        }
        
        if (strcmp(new_course, prev_course) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 125, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 125, new_course, value_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_course, new_course);
        }
    } else {
        // Display zero values when not positioning
        snprintf(new_lat, sizeof(new_lat), "0.000000");
        snprintf(new_lon, sizeof(new_lon), "0.000000");
        snprintf(new_speed, sizeof(new_speed), "0.0 km/h");
        snprintf(new_course, sizeof(new_course), "0.0\xF8");
        
        // Color settings: Use warning color when not positioning
        uint16_t lat_lon_color = COLOR_BAIDU;
        uint16_t value_color = COLOR_VALUE;
        
        // Update display only when value changes or status changes
        if (strcmp(new_lat, prev_lat) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 50, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 50, new_lat, lat_lon_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_lat, new_lat);
        }
        
        if (strcmp(new_lon, prev_lon) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 75, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 75, new_lon, lat_lon_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_lon, new_lon);
        }
        
        if (strcmp(new_speed, prev_speed) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 100, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 100, new_speed, value_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_speed, new_speed);
        }
        
        if (strcmp(new_course, prev_course) != 0 || prev_fix_state != gps_data.fix) {
            st7789_fill_rect(100, 125, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 125, new_course, value_color, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            strcpy(prev_course, new_course);
        }
    }
    
    // Date usually doesn't change frequently - Force display date, even if it's default value
    if (strcmp(gps_data.datestamp, prev_date) != 0 || strlen(prev_date) == 0) {
        st7789_fill_rect(100, 150, 130, 18, COLOR_BACKGROUND);
        st7789_draw_string(100, 150, gps_data.datestamp, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        strcpy(prev_date, gps_data.datestamp);
        
        if (enable_debug) {
            printf("Display date: %s\n", gps_data.datestamp);
        }
    }
    
    // Time changes every second
    if (strcmp(gps_data.timestamp, prev_time) != 0) {
        st7789_fill_rect(100, 175, 130, 18, COLOR_BACKGROUND);
        st7789_draw_string(100, 175, gps_data.timestamp, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
        strcpy(prev_time, gps_data.timestamp);
    }
    
    // Update positioning status tracking variable
    prev_fix_state = gps_data.fix;
}

/**
 * @brief Get data from GPS module and update gps_data structure
 * @return Whether a valid GPS data is successfully obtained
 */
static bool update_gps_data_from_module(void) {
    bool got_valid_data = false;
    bool got_time_data = false;
    
    // Prevent unexpected situations: Set a flag so that if an exception occurs during processing, it can be recovered
    static bool is_recovering = false;
    static uint32_t last_recovery_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // If an error occurred before and is still in recovery period (within 5 seconds)
    if (is_recovering && (current_time - last_recovery_time < 5000)) {
        // Still in recovery period, continue using previous data
        sleep_ms(100);
        return false;
    } else {
        // Recovery period ended or not in recovery state
        is_recovering = false;
    }
    
    // Try multiple times to get GPS data until a valid RMC sentence is found
    int max_attempts = 3; // Reduce the number of attempts to avoid long blocking
    GNRMC gnrmc_data = {0};
    
    for (int i = 0; i < max_attempts; i++) {
        // Get basic GPS data
        gnrmc_data = vendor_gps_get_gnrmc();
        
        // Check if valid time data is obtained
        if (gnrmc_data.Time_H > 0 || gnrmc_data.Time_M > 0 || gnrmc_data.Time_S > 0) {
            got_time_data = true;
            
            // Check if valid positioning data is obtained
            // Strictly verify coordinates: Status is 1 and non-zero coordinates are considered valid
            if (gnrmc_data.Status == 1) {
                // Coordinates are non-zero, if GGA sentence is used, it is not necessary to strictly verify latitude and longitude greater than 1
                if (fabs(gnrmc_data.Lat) > 0.0001 && fabs(gnrmc_data.Lon) > 0.0001) {
                    if (enable_debug) {
                        printf("Valid GPS data: status=%d, latitude=%.6f, longitude=%.6f\n", 
                              gnrmc_data.Status, gnrmc_data.Lat, gnrmc_data.Lon);
                    }
                    got_valid_data = true;
                    break;
                } else {
                    if (enable_debug) {
                        printf("GPS status is valid but coordinates are close to zero: latitude=%.6f, longitude=%.6f\n", 
                              gnrmc_data.Lat, gnrmc_data.Lon);
                    }
                }
            }
        }
        
        // If no valid data is obtained, wait briefly before trying again
        sleep_ms(50); // Reduce waiting time to improve responsiveness
    }
    
    // Increment packet count
    packet_count++;
    
    // Update time and date - Update time information as long as valid time is obtained
    if (got_time_data) {
        // To prevent exceptions from occurring during GPS data processing, all operations are placed in a recoverable block
        bool process_error = false;
        
        // Get converted coordinates
        Coordinates baidu_coords = {0};
        Coordinates google_coords = {0};
        
        // Try to get map coordinates conversion
        if (!process_error) {
            baidu_coords = vendor_gps_get_baidu_coordinates();
            google_coords = vendor_gps_get_google_coordinates();
        }
        
        // Determine if positioning is successful - Use loose verification, only Status is 1 and coordinates are non-zero
        bool has_fix = (gnrmc_data.Status == 1 && 
                      fabs(gnrmc_data.Lat) > 0.0001 && 
                      fabs(gnrmc_data.Lon) > 0.0001);
        
        // Always update time information, regardless of whether positioning is successful
        sprintf(gps_data.timestamp, "%02d:%02d:%02d", 
                gnrmc_data.Time_H, gnrmc_data.Time_M, gnrmc_data.Time_S);
        
        // Update date information (if any)
        if (gnrmc_data.Date[0] != '\0') {
            strcpy(gps_data.datestamp, gnrmc_data.Date);
            if (enable_debug) {
                printf("GPS date obtained: %s\n", gps_data.datestamp);
            }
        } else if (gps_data.datestamp[0] == '\0') {
            // Only use default value when GPS module has not provided date and local date is empty
            strcpy(gps_data.datestamp, "0000-00-00");
            if (enable_debug) {
                printf("GPS date not detected, using default value\n");
            }
        }
        
        if (has_fix && !process_error) {
            // Additional debug information, display verified coordinates
            if (enable_debug) {
                printf("Verified valid coordinates: latitude=%0.6f, longitude=%0.6f\n", gnrmc_data.Lat, gnrmc_data.Lon);
            }
            
            // Update position data when positioning is successful
            gps_data.latitude = gnrmc_data.Lat;
            gps_data.longitude = gnrmc_data.Lon;
            gps_data.speed = gnrmc_data.Speed;
            gps_data.course = gnrmc_data.Course;
            gps_data.altitude = gnrmc_data.Altitude;
            
            // Baidu and Google coordinates
            gps_data.baidu_lat = baidu_coords.Lat;
            gps_data.baidu_lon = baidu_coords.Lon;
            gps_data.google_lat = google_coords.Lat;
            gps_data.google_lon = google_coords.Lon;
            
            // L76X does not provide satellite count and HDOP, use simulated values
            gps_data.satellites = 6 + (rand() % 3); // Reduce maximum satellite count to fit display
            gps_data.hdop = 0.8 + (rand() % 16) / 10.0;
            
            valid_fix_count++;
        } else {
            // Not positioned or invalid coordinates, print debug information
            if (enable_debug && gnrmc_data.Status == 1) {
                printf("Warning: GPS module reports positioning success but coordinates are invalid (latitude=%0.6f, longitude=%0.6f)\n", 
                    gnrmc_data.Lat, gnrmc_data.Lon);
            }
            
            // Clear all position data when not positioned
            gps_data.latitude = 0.0;
            gps_data.longitude = 0.0;
            gps_data.speed = 0.0;
            gps_data.course = 0.0;
            gps_data.altitude = 0.0;
            gps_data.baidu_lat = 0.0;
            gps_data.baidu_lon = 0.0;
            gps_data.google_lat = 0.0;
            gps_data.google_lon = 0.0;
            
            // Low values for satellite count and HDOP when not positioned
            gps_data.satellites = 2 + (rand() % 2);
            gps_data.hdop = 2.5 + (rand() % 20) / 10.0;
        }
        
        // Update positioning status
        gps_data.fix = has_fix;
        
        // Check if an error occurred during processing
        if (process_error) {
            // Error occurred, mark recovery state
            if (enable_debug) {
                printf("GPS data processing exception occurred\n");
            }
            is_recovering = true;
            last_recovery_time = current_time;
        }
    } else {
        // If even time is not obtained, ensure all values are 0
        if (gps_data.timestamp[0] == '\0') {
            strcpy(gps_data.timestamp, "00:00:00");
        }
        // Note: Do not overwrite existing time and date values, so that the last valid time obtained can be maintained
        
        if (gps_data.datestamp[0] == '\0') {
            strcpy(gps_data.datestamp, "0000-00-00");
        }
        
        // If no positioning data is obtained and no time data is obtained, mark as not positioned
        if (!gps_data.fix) {
            gps_data.latitude = 0.0;
            gps_data.longitude = 0.0;
            gps_data.speed = 0.0;
            gps_data.course = 0.0;
            gps_data.altitude = 0.0;
            gps_data.baidu_lat = 0.0;
            gps_data.baidu_lon = 0.0;
            gps_data.google_lat = 0.0;
            gps_data.google_lon = 0.0;
            gps_data.satellites = 0;
            gps_data.hdop = 0.0;
        }
    }
    
    return got_valid_data;
}

/**
 * @brief Print detailed GPS data to serial port for debugging
 * @param force_print Whether to force print
 */
static void print_gps_debug_info(bool force_print) {
    // Print status information only when status changes or every 10 packets
    static bool last_status = false;
    bool status_changed = (last_status != gps_data.fix);
    bool should_print = status_changed || force_print || (packet_count % 10 == 0);
    
    if (should_print) {
        // Get original GNRMC data for detailed information printing
        GNRMC gnrmc_data = vendor_gps_get_gnrmc();
        
        // Simplified GPS data output
        if (gps_data.fix) {
            printf("GPS: coordinates=%0.6f,%0.6f speed=%.1fkm/h course=%.1f° time=%s\n", 
                   gps_data.latitude, gps_data.longitude, 
                   gps_data.speed, gps_data.course,
                   gps_data.timestamp);
        } else {
            printf("GPS: Waiting for positioning... time=%s date=%s\n", 
                   gps_data.timestamp, gps_data.datestamp);
        }
        
        last_status = gps_data.fix;
    }
}

/**
 * @brief GPS display main function
 */
void vendor_gps_display_demo(void) {
    printf("Starting Vendor GPS display demo...\n");
    
    // Set debug log level
    vendor_gps_set_debug(enable_debug);
    printf("Debug mode: %s\n", enable_debug ? "Enabled" : "Disabled");
    
    // Initialize GPS
    bool gps_init_success = vendor_gps_init(GPS_UART_ID, GPS_BAUD_RATE, GPS_TX_PIN, GPS_RX_PIN, GPS_FORCE_PIN);
    if (!gps_init_success) {
        printf("GPS initialization failed, check connection and try again\n");
        return;
    }
    
    printf("GPS initialization succeeded, start receiving data\n");
    printf("UART%d pin: TX=%d, RX=%d, baud rate=%d\n", 
           GPS_UART_ID, GPS_TX_PIN, GPS_RX_PIN, GPS_BAUD_RATE);
    
    // Send setting command - Configure NMEA output
    printf("Sending NMEA output configuration command...\n");
    
    // Modify command to enable both RMC and GGA sentence output, improve output frequency
    vendor_gps_send_command("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    sleep_ms(100);
    
    // Set update frequency to 1Hz (1000ms)
    vendor_gps_send_command("$PMTK220,1000");
    sleep_ms(500);
    
    printf("Note: Program has been enhanced, now able to parse GNRMC/GPRMC or GNGGA/GPGGA sentences\n");
    printf("      Speed and course information from RMC sentence, coordinate information supports both sentences\n");
    
    // Set seed for random number generator (for satellite signal graph simulation)
    srand(time_us_32());
    
    // Draw UI framework
    draw_gps_ui_frame();
    
    // Immediately try to get GPS data
    printf("\nInitial state...\n");
    bool got_data = update_gps_data_from_module();
    print_gps_debug_info(true); // Force print first data
    update_gps_display();
    
    if (!got_data) {
        printf("Warning: Initialization failed to obtain valid GPS data, please check GPS module connection\n");
    }
    
    // Main loop
    uint32_t last_gps_update = 0;    // GPS data update timer
    uint32_t last_time_update = 0;   // Time display update timer
    uint32_t last_blink = 0;         // Indicator blink timer
    bool indicator_on = false;
    
    // Manual time display update variable - Use current system time as initial value
    uint8_t second = 0;
    uint8_t minute = 0;
    uint8_t hour = 8;  // Default start from 8 o'clock
    
    // Get current GPS time (if any)
    GNRMC gnrmc_data = vendor_gps_get_gnrmc();
    if (gnrmc_data.Time_H > 0 || gnrmc_data.Time_M > 0 || gnrmc_data.Time_S > 0) {
        second = gnrmc_data.Time_S;
        minute = gnrmc_data.Time_M;
        hour = gnrmc_data.Time_H;
    }
    
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // Update GPS data every second
        if (current_time - last_gps_update >= 1000) {
            // Set a maximum timeout to prevent update_gps_data_from_module from blocking too long
            absolute_time_t gps_update_timeout = make_timeout_time_ms(500);
            bool update_error = false;
            
            // Get data from GPS module
            bool got_data = update_gps_data_from_module();
            
            // Synchronize local time variable (if valid time is obtained)
            if (got_data) {
                GNRMC gnrmc_data = vendor_gps_get_gnrmc();
                if (gnrmc_data.Time_H > 0 || gnrmc_data.Time_M > 0 || gnrmc_data.Time_S > 0) {
                    second = gnrmc_data.Time_S;
                    minute = gnrmc_data.Time_M;
                    hour = gnrmc_data.Time_H;
                }
            }
            
            // Print debug information to serial port
            print_gps_debug_info(false);
            
            // Update LCD display
            update_gps_display();
            
            // If GPS update timeout or exception occurs, revert to safe state
            if (time_reached(gps_update_timeout) || update_error) {
                if (enable_debug) {
                    printf("GPS data update timeout or error, use local time\n");
                }
                
                // Use local clock to update time display
                char new_time[9]; // HH:MM:SS
                sprintf(new_time, "%02d:%02d:%02d", hour, minute, second);
                st7789_fill_rect(100, 175, 130, 18, COLOR_BACKGROUND);
                st7789_draw_string(100, 175, new_time, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            }
            
            last_gps_update = current_time;
        }
        
        // Update time display every second - Manually maintain time
        if (current_time - last_time_update >= 1000) {
            // Update local time
            second++;
            if (second >= 60) {
                second = 0;
                minute++;
                if (minute >= 60) {
                    minute = 0;
                    hour++;
                    if (hour >= 24) {
                        hour = 0;
                    }
                }
            }
            
            // Format time string
            char new_time[9]; // HH:MM:SS
            sprintf(new_time, "%02d:%02d:%02d", hour, minute, second);
            
            // Update time display
            st7789_fill_rect(100, 175, 130, 18, COLOR_BACKGROUND);
            st7789_draw_string(100, 175, new_time, COLOR_VALUE, COLOR_BACKGROUND, FONT_SIZE_VALUE);
            
            if (enable_debug) {
                printf("Local time updated: %s\n", new_time);
            }
            
            last_time_update = current_time;
        }
        
        // Draw GPS activity indicator blink (top right corner), blink every second
        if (current_time - last_blink >= 1000) {  // Update every second
            // Switch indicator state
            indicator_on = !indicator_on;
            
            // Print debug information
            if (enable_debug) {
                printf("Indicator state: %s, time difference: %lu ms\n", 
                    indicator_on ? "On" : "Off", current_time - last_blink);
            }
            
            // Choose color based on positioning status and current indicator state
            uint16_t indicator_color;
            if (indicator_on) {
                // Ensure strict verification of GPS data validity, latitude and longitude must be greater than 1 to display green light
                // Here, use value in structure to determine, not just use fix flag
                bool valid_coordinates = (fabs(gps_data.latitude) > 1.0 && fabs(gps_data.longitude) > 1.0);
                indicator_color = (gps_data.fix && valid_coordinates) ? COLOR_GOOD : COLOR_WARNING;
            } else {
                indicator_color = ST7789_BLUE;
            }
            
            // Draw indicator
            st7789_fill_circle(SCREEN_WIDTH - 15, 15, 5, indicator_color);
            
            last_blink = current_time;
        }
        
        // Reduce sleep time to improve responsiveness to blink timer
        sleep_ms(1); // Minimum sleep time to ensure highest responsiveness
    }
}

/**
 * @brief Main function entry
 */
int main() {
    // Initialize standard library
    stdio_init_all();
    sleep_ms(2000);  // Wait for serial port to stabilize
    
    printf("\n=== Vendor GPS LCD Display Demo ===\n");
    
    // Initialize ST7789
    st7789_config_t config = {
        .spi_inst    = spi0,
        .spi_speed_hz = 40 * 1000 * 1000, // 40MHz
        .pin_din     = 19,
        .pin_sck     = 18,
        .pin_cs      = 17,
        .pin_dc      = 20,
        .pin_reset   = 15,
        .pin_bl      = 10,
        .width       = SCREEN_WIDTH,
        .height      = SCREEN_HEIGHT,
        .rotation    = 0
    };
    
    st7789_init(&config);
    st7789_set_rotation(2); // Portrait mode, rotate 180 degrees
    
    // Turn on LCD backlight
    printf("Turning on LCD backlight...\n");
    st7789_set_backlight(true);
    sleep_ms(500);
    
    // Start GPS display demo
    vendor_gps_display_demo();
    
    return 0;
} 