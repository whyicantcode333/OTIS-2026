@echo off
chcp 65001 > nul
setlocal

cd /d "%~dp0"

echo ========================================
echo   OTIS Task 02: Build, Test and Coverage
echo ========================================

if not exist build (
    cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    if errorlevel 1 (
        echo [ERROR] CMake configuration failed!
        pause
        exit /b 1
    )
)

cmake --build build
if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo Running unit tests...
ctest --test-dir build --output-on-failure
set TEST_STATUS=%ERRORLEVEL%

where gcovr >nul 2>nul
if %ERRORLEVEL%==0 (
    echo.
    echo Generating coverage report...
    gcovr -r .. --object-directory build --filter ".*/(task_01|task_02)/src/.*" --exclude-branches-by-pattern ".*" --html-details build/coverage.html
    echo Coverage report saved to: build\coverage.html
    start "" "build\coverage.html"
) else (
    echo.
    echo [INFO] gcovr not found in PATH, skipping HTML coverage generation.
)

pause
exit /b %TEST_STATUS%