
@echo off
echo ========================================
echo       CODE COVERAGE ANALYSIS
echo ========================================

echo.
echo Building project...

cmake -S . -B build
if errorlevel 1 exit /b 1

cmake --build build --config Debug
if errorlevel 1 exit /b 1

echo.
echo Running tests and collecting coverage...

OpenCppCoverage.exe ^
    --cover_children ^
    --sources "%CD%\src" ^
    --export_type "html:%CD%\coverage_report" ^
    -- ctest --test-dir build -C Debug --output-on-failure

if errorlevel 1 (
    echo Coverage analysis failed!
    pause
    exit /b 1
)

echo.
echo Coverage report generated!
start "" "coverage_report\index.html"

pause
