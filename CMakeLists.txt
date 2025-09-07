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

# =============================================================================
# GPS 模块库
# =============================================================================

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

# =============================================================================
# 显示模块库
# =============================================================================

# ST7789显示模块（原有）
add_library(st7789_display_module
    src/display/st7789/st7789.c
    src/display/st7789/st7789_gfx.c
    src/display/st7789/st7789_hal.c
    src/display/st7789/st7789_font.c
)

# 为ST7789模块添加私有包含目录
target_include_directories(st7789_display_module PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include/display/st7789
)

# 链接ST7789显示模块所需库
target_link_libraries(st7789_display_module
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_pwm
)

# ILI9488显示模块（新增）
add_library(ili9488_display_module
    src/display/ili9488/ili9488_driver.cpp
    src/display/ili9488/ili9488_ui.cpp
    src/display/ili9488/fonts/ili9488_font.cpp
    src/display/ili9488/hal/ili9488_hal.cpp
)

# 为ILI9488模块添加私有包含目录
target_include_directories(ili9488_display_module PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include/display/ili9488
)

# 链接ILI9488显示模块所需库
target_link_libraries(ili9488_display_module
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_pwm
    hardware_dma
)

# 为ILI9488模块设置C++编译属性
set_target_properties(ili9488_display_module PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# =============================================================================
# 示例可执行文件
# =============================================================================

# 厂商GPS基础示例
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

# 厂商GPS ST7789显示示例
add_executable(vendor_gps_display
    examples/vendor_gps_display.c
)

# 为vendor_gps_display添加包含目录
target_include_directories(vendor_gps_display PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include/display/st7789
)

# 链接厂商GPS ST7789显示示例所需库
target_link_libraries(vendor_gps_display
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_spi
    hardware_pwm
    pico_time
    vendor_gps_module
    st7789_display_module
)

# 为厂商GPS ST7789显示示例启用USB输出，禁用UART输出
pico_enable_stdio_usb(vendor_gps_display 1)
pico_enable_stdio_uart(vendor_gps_display 0)

# 创建额外文件(.uf2, .hex等)
pico_add_extra_outputs(vendor_gps_display)

# 厂商GPS ILI9488显示示例（新增）
add_executable(vendor_gps_ili9488
    examples/vendor_gps_ili9488.cpp
)

# 为vendor_gps_ili9488添加包含目录
target_include_directories(vendor_gps_ili9488 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include/display/ili9488
)

# 链接厂商GPS ILI9488显示示例所需库
target_link_libraries(vendor_gps_ili9488
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_spi
    hardware_pwm
    hardware_dma
    pico_time
    vendor_gps_module
    ili9488_display_module
)

# 为厂商GPS ILI9488显示示例启用USB输出，禁用UART输出
pico_enable_stdio_usb(vendor_gps_ili9488 1)
pico_enable_stdio_uart(vendor_gps_ili9488 0)

# 设置C++编译属性（因为使用了ILI9488 C++驱动）
set_target_properties(vendor_gps_ili9488 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 创建额外文件(.uf2, .hex等)
pico_add_extra_outputs(vendor_gps_ili9488)

# 厂商GPS ILI9488显示示例（优化版）
add_executable(vendor_gps_ili9488_optimized
    examples/vendor_gps_ili9488_optimized.cpp
)

# 为vendor_gps_ili9488_optimized添加包含目录
target_include_directories(vendor_gps_ili9488_optimized PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include/display/ili9488
)

# 链接厂商GPS ILI9488显示示例（优化版）所需库
target_link_libraries(vendor_gps_ili9488_optimized
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_spi
    hardware_pwm
    hardware_dma
    pico_time
    vendor_gps_module
    ili9488_display_module
)

# 为厂商GPS ILI9488显示示例（优化版）启用USB输出，禁用UART输出
pico_enable_stdio_usb(vendor_gps_ili9488_optimized 1)
pico_enable_stdio_uart(vendor_gps_ili9488_optimized 0)

# 设置C++编译属性（因为使用了ILI9488 C++驱动）
set_target_properties(vendor_gps_ili9488_optimized PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 创建额外文件(.uf2, .hex等)
pico_add_extra_outputs(vendor_gps_ili9488_optimized)

# 厂商GPS仪表盘显示示例
add_executable(vendor_gps_dashboard
    examples/vendor_gps_dashboard.cpp
)

# 为vendor_gps_dashboard添加包含目录
target_include_directories(vendor_gps_dashboard PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include/display/ili9488
)

# 链接厂商GPS仪表盘显示示例所需库
target_link_libraries(vendor_gps_dashboard
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_spi
    hardware_pwm
    hardware_dma
    pico_time
    vendor_gps_module
    ili9488_display_module
)

# 为厂商GPS仪表盘显示示例启用USB输出，禁用UART输出
pico_enable_stdio_usb(vendor_gps_dashboard 1)
pico_enable_stdio_uart(vendor_gps_dashboard 0)

# 设置C++编译属性（因为使用了ILI9488 C++驱动）
set_target_properties(vendor_gps_dashboard PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 创建额外文件(.uf2, .hex等)
pico_add_extra_outputs(vendor_gps_dashboard)

# =============================================================================
# 编译配置和优化选项
# =============================================================================

# 编译优化选项
add_compile_options(-Wall -Wextra -Wnull-dereference)
add_compile_options(-O2)  # 启用优化

# 为C++文件添加特定编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

# =============================================================================
# 打印构建信息
# =============================================================================

message(STATUS "LC76G_Pico项目配置完成")
message(STATUS "支持的可执行文件:")
message(STATUS "  - vendor_gps_example: GPS基础功能示例")
message(STATUS "  - vendor_gps_display: GPS + ST7789显示示例") 
message(STATUS "  - vendor_gps_ili9488: GPS + ILI9488显示示例")
message(STATUS "  - vendor_gps_ili9488_optimized: GPS + ILI9488显示示例（优化版）")
message(STATUS "  - vendor_gps_dashboard: GPS + ILI9488仪表盘示例（高科技风格）")
message(STATUS "C标准: ${CMAKE_C_STANDARD}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
