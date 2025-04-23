/**
 * @file vendor_gps_example.c
 * @brief L76X GPS module usage example - Vendor code version (optimized log output)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "gps/vendor_gps_parser.h"

// Configuration parameters
#define GPS_UART_ID    0          // Use UART0
#define GPS_TX_PIN     0          // GPIO0 (UART0 TX)
#define GPS_RX_PIN     1          // GPIO1 (UART0 RX)
#define GPS_FORCE_PIN  4          // GPIO4 (FORCE pin)
#define GPS_BAUD_RATE  115200     // GPS module baud rate set to 115200

// Main GPS data
static GNRMC gps_data = {0};
static uint32_t packet_count = 0;
static bool enable_debug = false;  // Whether to display detailed debug information

/**
 * @brief Main function
 */
int main() {
    // Initialize standard library (setup UART, etc.)
    stdio_init_all();
    sleep_ms(2000);  // Wait for UART to stabilize
    
    printf("\n=== L76X GPS Module Test - Optimized Version ===\n");
    printf("UART%d Pins: TX=%d, RX=%d, Baud Rate=%d\n", 
           GPS_UART_ID, GPS_TX_PIN, GPS_RX_PIN, GPS_BAUD_RATE);
    
    // Set debug log level
    vendor_gps_set_debug(enable_debug);
    
    // Initialize GPS
    vendor_gps_init(GPS_UART_ID, GPS_BAUD_RATE, GPS_TX_PIN, GPS_RX_PIN, GPS_FORCE_PIN);
    printf("GPS initialization complete, start receiving data...\n\n");
    
    // Send setup commands - Configure NMEA output
    vendor_gps_send_command("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    vendor_gps_send_command("$PMTK220,1000");
    
    // Main loop
    while (true) {
        // Get GPS data - using vendor code
        gps_data = vendor_gps_get_gnrmc();
        packet_count++;
        
        // Only print status information when status changes or every 10 packets
        static bool last_status = false;
        bool status_changed = (last_status != (gps_data.Status == 1));
        bool should_print = status_changed || (packet_count % 10 == 0);
        
        if (should_print) {
            printf("\n[Packet #%lu] ", packet_count);
            
            if (gps_data.Status) {
                printf("GPS Positioned ✓\n");
                
                // Print time and date
                printf("Time: %02d:%02d:%02d  Date: %s\n", 
                       gps_data.Time_H, gps_data.Time_M, gps_data.Time_S, 
                       gps_data.Date[0] ? gps_data.Date : "Unknown");
                
                // Print original NMEA format and converted decimal degree format
                printf("Latitude: %.6f%c → %.6f°\n", 
                       gps_data.Lat_Raw, gps_data.Lat_area, gps_data.Lat);
                printf("Longitude: %.6f%c → %.6f°\n", 
                       gps_data.Lon_Raw, gps_data.Lon_area, gps_data.Lon);
                
                // Only display speed and course when positioning is valid
                if (gps_data.Speed > 0 || gps_data.Course > 0) {
                    printf("Speed: %.1f km/h  Course: %.1f°\n", 
                           gps_data.Speed, gps_data.Course);
                }
                
                // Coordinate conversion
                Coordinates google_coords = vendor_gps_get_google_coordinates();
                Coordinates baidu_coords = vendor_gps_get_baidu_coordinates();
                
                // Only display converted coordinates when really needed
                if (enable_debug) {
                    printf("\nCoordinate conversion results:\n");
                    printf("Google Maps: %.6f, %.6f\n", 
                           google_coords.Lat, google_coords.Lon);
                    printf("Baidu Maps: %.6f, %.6f\n", 
                           baidu_coords.Lat, baidu_coords.Lon);
                }
                
                // Output Baidu Maps link - this is always useful
                printf("Baidu Maps: https://api.map.baidu.com/marker?location=%.6f,%.6f&title=GPS&content=Current Location&output=html\n",
                       baidu_coords.Lat, baidu_coords.Lon);
            } else {
                printf("Waiting for positioning... ✗\n");
                
                // Only print more information in debug mode
                if (enable_debug) {
                    printf("Time: %02d:%02d:%02d\n", 
                           gps_data.Time_H, gps_data.Time_M, gps_data.Time_S);
                }
            }
            
            last_status = (gps_data.Status == 1);
        }
        
        sleep_ms(1000);  // Update once per second
    }
    
    return 0;
} 