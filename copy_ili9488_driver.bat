@echo off
echo 正在从2048项目复制ILI9488显示驱动到当前项目...

REM 设置源目录和目标目录
set SOURCE_DIR=2048
set TARGET_DIR=.

REM 复制显示驱动适配器
echo 复制显示驱动适配器...
copy "%SOURCE_DIR%\include\display\adapters\ili9488_adapter.hpp" "%TARGET_DIR%\include\display\adapters\"
copy "%SOURCE_DIR%\src\display\adapters\ili9488_adapter.cpp" "%TARGET_DIR%\src\display\adapters\"

REM 复制显示驱动基础类
echo 复制显示驱动基础类...
copy "%SOURCE_DIR%\include\display\display_driver.hpp" "%TARGET_DIR%\include\display\"
copy "%SOURCE_DIR%\include\display\display_manager.hpp" "%TARGET_DIR%\include\display\"
copy "%SOURCE_DIR%\src\display\display_manager.cpp" "%TARGET_DIR%\src\display\"

REM 复制硬件驱动头文件
echo 复制硬件驱动头文件...
copy "%SOURCE_DIR%\include\hardware\display\ili9488\ili9488_driver.hpp" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\ili9488_colors.hpp" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\ili9488_font.hpp" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\ili9488_hal.hpp" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\ili9488_ui.hpp" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\pico_ili9488_gfx.hpp" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\pico_ili9488_gfx.inl" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\flash_font_cache.hpp" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\hybrid_font_renderer.hpp" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\hybrid_font_renderer.inl" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\hybrid_font_system.hpp" "%TARGET_DIR%\include\display\ili9488\"
copy "%SOURCE_DIR%\include\hardware\display\ili9488\unicode_ranges.h" "%TARGET_DIR%\include\display\ili9488\"

REM 复制硬件驱动实现文件
echo 复制硬件驱动实现文件...
copy "%SOURCE_DIR%\src\hardware\display\ili9488\ili9488_driver.cpp" "%TARGET_DIR%\src\display\ili9488\"
copy "%SOURCE_DIR%\src\hardware\display\ili9488\ili9488_font.cpp" "%TARGET_DIR%\src\display\ili9488\"
copy "%SOURCE_DIR%\src\hardware\display\ili9488\ili9488_hal.cpp" "%TARGET_DIR%\src\display\ili9488\"
copy "%SOURCE_DIR%\src\hardware\display\ili9488\ili9488_ui.cpp" "%TARGET_DIR%\src\display\ili9488\"
copy "%SOURCE_DIR%\src\hardware\display\ili9488\pico_ili9488_gfx.cpp" "%TARGET_DIR%\src\display\ili9488\"
copy "%SOURCE_DIR%\src\hardware\display\ili9488\flash_font_cache.cpp" "%TARGET_DIR%\src\display\ili9488\"
copy "%SOURCE_DIR%\src\hardware\display\ili9488\hybrid_font_system.cpp" "%TARGET_DIR%\src\display\ili9488\"

REM 复制字体文件
echo 复制字体文件...
copy "%SOURCE_DIR%\src\display\ili9488\fonts\ili9488_font.cpp" "%TARGET_DIR%\src\display\ili9488\fonts\"

REM 复制HAL文件
echo 复制HAL文件...
copy "%SOURCE_DIR%\src\display\ili9488\hal\ili9488_hal.cpp" "%TARGET_DIR%\src\display\ili9488\hal\"

REM 复制配置文件
echo 复制配置文件...
copy "%SOURCE_DIR%\config\ili9488_config.json" "%TARGET_DIR%\config\"

echo.
echo ILI9488显示驱动复制完成！
echo.
echo 已复制的文件包括：
echo - 显示驱动适配器 (ili9488_adapter)
echo - 显示驱动基础类 (display_driver, display_manager)
echo - 硬件驱动头文件和实现
echo - 字体和UI相关文件
echo - 配置文件
echo.
echo 请检查复制的文件并根据需要调整包含路径。
pause
