# LC76G_Pico GPS Tracker

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-brightgreen.svg)
![Version](https://img.shields.io/badge/version-1.0.0-orange.svg)

English | [中文](README.zh.md)

## Overview

LC76G_Pico is an open-source GPS tracking project based on the Raspberry Pi Pico microcontroller and the L76X GPS module. This project provides a complete solution for retrieving, processing, and displaying GPS data, making it ideal for location tracking applications, navigation systems, and IoT devices requiring geospatial functionality.

## Features

- **Comprehensive GPS Data Processing**: Accurately extracts and processes geographic coordinates, altitude, speed, and course information from NMEA sentences.
- **Multiple Coordinate System Support**: Built-in conversion functions for WGS-84, GCJ-02 (Google Maps), and BD-09 (Baidu Maps) coordinate systems.
- **Modular Architecture**: Clean separation between hardware drivers and application logic for enhanced extensibility.
- **Low Power Operation**: Support for standby modes and power management features of the L76X GPS module.
- **Flexible Configuration**: Configurable UART, GPIO pins, and GPS module parameters.
- **Robust NMEA Parser**: Reliably parses GPGGA and GPRMC sentences with proper validation and error handling.
- **Mock Data Support**: Includes simulation capabilities for testing without physical GPS hardware.

## Hardware Requirements

- Raspberry Pi Pico/Pico W
- L76X GPS Module
- Optional: ST7789 Display for visualization (240×320)
- UART connection cables
- Power supply (USB or battery)

## Wiring Diagram

```
┌─────────────────┐                  ┌─────────────────┐
│                 │                  │                 │
│  Raspberry Pi   │                  │    L76X GPS     │
│     Pico        │                  │     Module      │
│                 │                  │                 │
│  GPIO 0 (TX)    │────────────────→│  RX             │
│  GPIO 1 (RX)    │←────────────────│  TX             │
│  VCC            │────────────────→│  VCC            │
│  GND            │←───────────────→│  GND            │
│                 │                  │                 │
└─────────────────┘                  └─────────────────┘
```

## Software Architecture

The project is structured into the following components:

- **GPS Parser Module**: Core library for GPS data extraction and processing
- **Display Module**: Optional UI functionality for rendering GPS information
- **Example Applications**: Demonstration programs showing library usage

## Getting Started

### Prerequisites

- [Raspberry Pi Pico C/C++ SDK](https://github.com/raspberrypi/pico-sdk)
- [CMake](https://cmake.org/) (minimum version 3.13)
- C/C++ compiler (GCC or Clang)
- Git

### Installation

1. Clone this repository:
```bash
git clone https://github.com/yourusername/LC76G_Pico.git
cd LC76G_Pico
```

2. Set up the Pico SDK (if not done already):
```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

3. Build the project:
```bash
mkdir build
cd build
cmake ..
make
```

4. Upload to your Pico:
   - Press and hold the BOOTSEL button on the Pico
   - Connect it to your computer while holding the button
   - Release the button after connecting
   - Copy the generated `.uf2` file (e.g., `gps_example.uf2`) to the mounted Pico drive

### Example Usage

The basic example demonstrates receiving and processing GPS data:

```c
#include "gps/gps_parser.h"

int main() {
    stdio_init_all();
    
    // Initialize GPS module with default UART0 configuration (GPIO0-TX, GPIO1-RX)
    gps_init(GPS_DEFAULT_UART_ID, GPS_DEFAULT_BAUD_RATE, 
             GPS_DEFAULT_TX_PIN, GPS_DEFAULT_RX_PIN, -1, -1);
    
    while (true) {
        // Parse incoming GPS data
        if (gps_parse_data()) {
            // Get the processed GPS data
            gps_data_t data = gps_get_data();
            
            // Do something with the data
            printf("Position: %.6f%c, %.6f%c\n", 
                   fabsf(data.latitude), data.lat_dir,
                   fabsf(data.longitude), data.lon_dir);
        }
        
        sleep_ms(100);
    }
    
    return 0;
}
```

## API Reference

### Core Functions

| Function | Description |
|----------|-------------|
| `gps_init()` | Initialize the GPS module |
| `gps_parse_data()` | Process incoming NMEA sentences |
| `gps_get_data()` | Retrieve the current GPS data structure |
| `gps_get_position()` | Get latitude and longitude values |
| `gps_has_fix()` | Check if valid positioning data is available |

### Specialized Functions

| Function | Description |
|----------|-------------|
| `gps_get_baidu_coordinates()` | Convert to Baidu Maps format |
| `gps_get_google_coordinates()` | Convert to Google Maps format |
| `gps_hot_start()` | Perform a quick GPS restart |
| `gps_cold_start()` | Perform a full GPS reset |
| `gps_set_standby_mode()` | Enter or exit low-power mode |

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the project
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [NMEA Standard](https://www.nmea.org/)
- [WGS-84 to GCJ-02 Conversion](https://github.com/googollee/eviltransform)
