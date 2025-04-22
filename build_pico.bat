@echo off
echo  start build...

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
pause 