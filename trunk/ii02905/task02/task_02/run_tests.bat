@echo off
chcp 65001 > nul

echo ====================================
echo       Task 02 Variant 5
echo          Running Tests
echo ====================================

cd /d "%~dp0"

echo.
echo [1/3] Configuring project...
cmake -S . -B build
if errorlevel 1 (
    echo Configuration failed!
    pause
    exit /b 1
)

echo.
echo [2/3] Building project...
cmake --build build
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Running tests...
ctest --test-dir build -C Debug --output-on-failure
if errorlevel 1 (
    echo.
    echo Some tests failed!
    pause
    exit /b 1
)

echo.
echo ====================================
echo       ALL TESTS PASSED!
echo ====================================

pause