@echo off
echo  start build...

rem 设置Pico SDK路径
set PICO_SDK_PATH=C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\pico-sdk
echo  set PICO_SDK_PATH to %PICO_SDK_PATH%

rem 复制pico_sdk_import.cmake文件(如果存在)
if exist "%PICO_SDK_PATH%\external\pico_sdk_import.cmake" (
    echo  copying pico_sdk_import.cmake from SDK external directory...
    copy "%PICO_SDK_PATH%\external\pico_sdk_import.cmake" .
) else if exist "%PICO_SDK_PATH%\pico_sdk_import.cmake" (
    echo  copying pico_sdk_import.cmake from SDK root...
    copy "%PICO_SDK_PATH%\pico_sdk_import.cmake" .
)

if exist build (
    echo  clean old build files...
    rd /s /q build
)

mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make -j8

if %ERRORLEVEL% NEQ 0 (
  echo  build failed!
  pause
  exit /b %ERRORLEVEL%
)

echo  build success!
echo UF2 file has been generated in the build directory
cd ..