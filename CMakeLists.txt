cmake_minimum_required(VERSION 3.13)

# 包含Pico SDK cmake文件
include(pico_sdk_import.cmake)

# 项目名称和语言设置
project(LC76G_Pico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 初始化Pico SDK
pico_sdk_init()

# 添加包含目录
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

# 厂商GPS模块
add_library(vendor_gps_module
    src/gps/vendor_gps_parser.c
)

# 链接厂商GPS模块所需库
target_link_libraries(vendor_gps_module
    pico_stdlib
    hardware_uart
    hardware_gpio
    pico_time
)

# 显示模块
add_library(display_module
    src/display/st7789.c
    src/display/st7789_gfx.c
    src/display/st7789_hal.c
    src/display/st7789_font.c
)

# 链接显示模块所需库
target_link_libraries(display_module
    pico_stdlib
    hardware_spi
    hardware_gpio
)

# 添加厂商GPS示例可执行文件
add_executable(vendor_gps_example
    examples/vendor_gps_example.c
)

# 链接厂商GPS示例所需库
target_link_libraries(vendor_gps_example
    pico_stdlib
    hardware_uart
    hardware_gpio
    pico_time
    vendor_gps_module
)

# 为厂商GPS示例启用USB输出，禁用UART输出
pico_enable_stdio_usb(vendor_gps_example 1)
pico_enable_stdio_uart(vendor_gps_example 0)

# 创建额外文件(.uf2, .hex等)
pico_add_extra_outputs(vendor_gps_example)

# 添加厂商GPS显示示例可执行文件
add_executable(vendor_gps_display
    examples/vendor_gps_display.c
)

# 链接厂商GPS显示示例所需库
target_link_libraries(vendor_gps_display
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_spi
    pico_time
    vendor_gps_module
    display_module
)

# 为厂商GPS显示示例启用USB输出，禁用UART输出
pico_enable_stdio_usb(vendor_gps_display 1)
pico_enable_stdio_uart(vendor_gps_display 0)

# 创建额外文件(.uf2, .hex等)
pico_add_extra_outputs(vendor_gps_display)
