/**
 * @file vendor_gps_parser.h
 * @brief Porting of vendor-provided GPS parsing functionality
 */

#ifndef VENDOR_GPS_PARSER_H
#define VENDOR_GPS_PARSER_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"

// LC76G Enhanced GPS data structure supporting multiple NMEA formats
typedef struct {
    // Basic positioning data
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
    
    // LC76G specific fields
    uint8_t Quality;    // GPS quality (0=invalid, 1=GPS SPS, 2=differential/SBAS)
    uint8_t Satellites; // Number of satellites used
    double HDOP;        // Horizontal dilution of precision
    double PDOP;        // Position dilution of precision
    double VDOP;        // Vertical dilution of precision
    char Mode;          // Mode indicator (A=autonomous, D=differential, N=no fix)
    char NavStatus;     // Navigation status (V=invalid, A=valid)
} GNRMC;

// LC76G PAIR command response structure
typedef struct {
    uint16_t CommandID; // Command ID (e.g., 050, 062, 864)
    uint8_t Result;     // Result (0=success, non-zero=error)
    bool Valid;         // Whether response is valid
} PAIRResponse;

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

// LC76G Enhanced Functions

/**
 * @brief Send PAIR command to LC76G module
 * @param command_id Command ID (e.g., 050, 062, 864)
 * @param params Command parameters (comma-separated)
 * @return PAIR response structure
 */
PAIRResponse vendor_gps_send_pair_command(uint16_t command_id, const char* params);

/**
 * @brief Set LC76G positioning rate
 * @param rate_ms Rate in milliseconds (100-1000)
 * @return Whether command was successful
 */
bool vendor_gps_set_positioning_rate(uint16_t rate_ms);

/**
 * @brief Set LC76G NMEA message output rate
 * @param message_type Message type (0=GGA, 1=GLL, 2=GSA, 3=GSV, 4=RMC, 5=VTG)
 * @param output_rate Output rate (0=disable, 1-20=every N fixes)
 * @return Whether command was successful
 */
bool vendor_gps_set_nmea_output_rate(uint8_t message_type, uint8_t output_rate);

/**
 * @brief Set LC76G baud rate
 * @param baud_rate Baud rate (9600, 115200, 230400, 460800, 921600, 3000000)
 * @return Whether command was successful
 */
bool vendor_gps_set_baud_rate(uint32_t baud_rate);

/**
 * @brief Perform LC76G cold start
 * @return Whether command was successful
 */
bool vendor_gps_cold_start(void);

/**
 * @brief Perform LC76G hot start
 * @return Whether command was successful
 */
bool vendor_gps_hot_start(void);

/**
 * @brief Save LC76G configuration to flash
 * @return Whether command was successful
 */
bool vendor_gps_save_config(void);

/**
 * @brief Set LC76G satellite systems
 * @param gps Enable GPS (1=enabled, 0=disabled)
 * @param glonass Enable GLONASS (1=enabled, 0=disabled)
 * @param galileo Enable Galileo (1=enabled, 0=disabled)
 * @param bds Enable BDS (1=enabled, 0=disabled)
 * @param qzss Enable QZSS (1=enabled, 0=disabled)
 * @return Whether command was successful
 */
bool vendor_gps_set_satellite_systems(uint8_t gps, uint8_t glonass, uint8_t galileo, uint8_t bds, uint8_t qzss);

/**
 * @brief Get LC76G satellite count from GSV messages
 * @return Number of satellites in view
 */
uint8_t vendor_gps_get_satellite_count(void);

/**
 * @brief Get LC76G signal strength from GSV messages
 * @return Average signal strength (0-100)
 */
uint8_t vendor_gps_get_signal_strength(void);

#endif /* VENDOR_GPS_PARSER_H */ 