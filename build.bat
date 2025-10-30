@echo off
echo ========================================
echo USB Storage Monitor - Build Script
echo ========================================
echo.

where cl >nul 2>nul
if errorlevel 1 (
    echo ERROR: Visual Studio compiler not found.
    echo Please run from Visual Studio Developer Command Prompt.
    pause
    exit /b 1
)

if not exist build mkdir build
cd build

echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo ERROR: CMake configuration failed.
    pause
    exit /b 1
)

echo Building project...
cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Build failed.
    pause
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Executable: build\Release\USBStorageMonitor.exe
echo.
echo To run:
echo 1. Run as Administrator
echo 2. Access http://localhost:8080
echo 3. Login: admin / secure123
echo.
pause
