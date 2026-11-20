@echo off
cd /d "%~dp0"

echo   Starting configuration
echo ----------------------------
cmake -S . -B build
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Configuration failed!
    pause
    exit /b 1
)
echo [OK] Configuration completed.

echo.
echo   Building project
echo -----------------------
cmake --build build --config Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo   Build completed
pause