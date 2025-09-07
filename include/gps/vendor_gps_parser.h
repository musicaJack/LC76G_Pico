/**
 * @file vendor_gps_parser.h
 * @brief Porting of vendor-provided GPS parsing functionality
 */

#ifndef VENDOR_GPS_PARSER_H
#define VENDOR_GPS_PARSER_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"

// Original vendor code data structure, enhanced version
typedef struct {
    // Original structure
    double Lon;         // Longitude (decimal degree format)
    double Lat;         // Latitude (decimal degree format)
    char Lon_area;      // Longitude area ('E'/'W')
    char Lat_area;      // Latitude area ('N'/'S')
    uint8_t Time_H;     // Hour
    uint8_t Time_M;     // Minute
    uint8_t Time_S;     // Second
    uint8_t Status;     // Positioning status (1:positioning successful 0:positioning failed)
    
    // Enhanced fields
    double Lon_Raw;     // Original NMEA format longitude (dddmm.mmmm)
    double Lat_Raw;     // Original NMEA format latitude (ddmm.mmmm)
    double Speed;       // Speed (kilometers/hour)
    double Course;      // Course (degrees)
    char Date[11];      // Date (YYYY-MM-DD format)
    double Altitude;    // Altitude (meters)
} GNRMC;

// Coordinates structure
typedef struct {
    double Lon;         // Longitude
    double Lat;         // Latitude
} Coordinates;

/**
 * @brief Set whether to output detailed debug logs
 * @param enable Whether to enable
 */
void vendor_gps_set_debug(bool enable);

/**
 * @brief Initialize the vendor version of GPS module
 * @param uart_id UART ID (0 means UART0)
 * @param baud_rate Baud rate
 * @param tx_pin TX pin
 * @param rx_pin RX pin
 * @param force_pin FORCE pin (set to -1 if not used)
 * @return Whether initialization was successful
 */
bool vendor_gps_init(uint uart_id, uint baud_rate, uint tx_pin, uint rx_pin, int force_pin);

/**
 * @brief Send command to GPS module
 * @param data Command string, no need to add checksum
 */
void vendor_gps_send_command(const char *data);

/**
 * @brief Make GPS module exit backup mode
 */
void vendor_gps_exit_backup_mode(void);

/**
 * @brief Get GPS GNRMC data
 * @return GNRMC data structure
 */
GNRMC vendor_gps_get_gnrmc(void);

/**
 * @brief Get GPS coordinates in Baidu Map format
 * @return Baidu Map coordinates
 */
Coordinates vendor_gps_get_baidu_coordinates(void);

/**
 * @brief Get GPS coordinates in Google Map format
 * @return Google Map coordinates
 */
Coordinates vendor_gps_get_google_coordinates(void);

#endif /* VENDOR_GPS_PARSER_H */ 