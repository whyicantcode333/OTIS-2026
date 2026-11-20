@echo off
chcp 65001 > nul

echo ====================================
echo       Building Task 02 Variant 5
echo ====================================

cd /d "%~dp0"

cmake -S . -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5

if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

cmake --build build

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
pause
