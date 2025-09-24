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
    hardware_i2c
    hardware_gpio
    pico_time
)

# LC76G I2C适配器模块
add_library(lc76g_i2c_adaptor
    src/gps/lc76g_i2c_adaptor.c
)

# 链接LC76G I2C适配器所需库
target_link_libraries(lc76g_i2c_adaptor
    pico_stdlib
    hardware_i2c
    hardware_gpio
    pico_time
    pico_sync
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
# MicroSD 模块库
# =============================================================================

# 添加MicroSD库的包含目录
include_directories(${CMAKE_CURRENT_LIST_DIR}/demo/MicroSD-Pico/include)

# 添加pico_fatfs子目录
add_subdirectory(lib/pico_fatfs)

# MicroSD库 (使用demo目录中的实现)
add_library(microsd_module
    demo/MicroSD-Pico/src/rw_sd.cpp
    demo/MicroSD-Pico/src/storage_device.cpp
)

# 为MicroSD模块添加私有包含目录
target_include_directories(microsd_module PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/demo/MicroSD-Pico/include
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_fatfs
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_fatfs/fatfs
)

# 链接MicroSD模块所需库
target_link_libraries(microsd_module
    pico_stdlib
    hardware_spi
    hardware_gpio
    pico_time
    pico_sync
    pico_fatfs
)

# 为MicroSD模块设置C++编译属性
set_target_properties(microsd_module PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# =============================================================================
# GPS日志记录器模块
# =============================================================================

# GPS日志记录器库
add_library(gps_logger_module
    src/gps/gps_logger.cpp
)

# 为GPS日志记录器模块添加私有包含目录
target_include_directories(gps_logger_module PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/demo/MicroSD-Pico/include
)

# 链接GPS日志记录器模块所需库
target_link_libraries(gps_logger_module
    pico_stdlib
    hardware_i2c
    hardware_gpio
    hardware_spi
    pico_time
    pico_sync
    lc76g_i2c_adaptor
    microsd_module
)

# 为GPS日志记录器模块设置C++编译属性
set_target_properties(gps_logger_module PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# =============================================================================
# 示例可执行文件
# =============================================================================

# 厂商GPS ILI9488显示示例（优化版）- 基于LC76G I2C适配器
add_executable(vendor_gps_ili9488_optimized
    examples/vendor_gps_ili9488_optimized.cpp
)

# 为vendor_gps_ili9488_optimized添加包含目录
target_include_directories(vendor_gps_ili9488_optimized PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include/display/ili9488
)

# 链接厂商GPS ILI9488显示示例所需库
target_link_libraries(vendor_gps_ili9488_optimized
    pico_stdlib
    hardware_i2c
    hardware_gpio
    hardware_spi
    hardware_pwm
    hardware_dma
    pico_time
    pico_sync
    lc76g_i2c_adaptor
    ili9488_display_module
    microsd_module
    gps_logger_module
)

# 为厂商GPS ILI9488显示示例启用USB输出，禁用UART输出
pico_enable_stdio_usb(vendor_gps_ili9488_optimized 1)
pico_enable_stdio_uart(vendor_gps_ili9488_optimized 0)

# 设置C++编译属性
set_target_properties(vendor_gps_ili9488_optimized PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 创建额外文件(.uf2, .hex等)
pico_add_extra_outputs(vendor_gps_ili9488_optimized)

# GPS SD卡日志记录演示程序
add_executable(gps_sd_logger_demo
    examples/gps_sd_logger_demo.cpp
)

# 链接GPS SD卡日志记录演示程序所需库
target_link_libraries(gps_sd_logger_demo
    pico_stdlib
    hardware_i2c
    hardware_gpio
    hardware_spi
    pico_time
    pico_sync
    microsd_module
    lc76g_i2c_adaptor
    gps_logger_module
)

# 为GPS SD卡日志记录演示程序启用USB输出，禁用UART输出
pico_enable_stdio_usb(gps_sd_logger_demo 1)
pico_enable_stdio_uart(gps_sd_logger_demo 0)

# 设置C++编译属性
set_target_properties(gps_sd_logger_demo PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 创建额外文件(.uf2, .hex等)
pico_add_extra_outputs(gps_sd_logger_demo)



# 注意：lc76g_dashboard_test.cpp 已删除

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
