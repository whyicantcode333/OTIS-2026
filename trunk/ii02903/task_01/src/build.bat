@echo off
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake not found. Install CMake and tick "Add CMake to the system PATH".
    pause
    exit /b 1
)

rem If MinGW g++ is installed use it, otherwise let CMake pick Visual Studio
set GEN=
where g++ >nul 2>nul
if not errorlevel 1 set GEN=-G "MinGW Makefiles"

if exist build rmdir /s /q build

echo Starting configuration
echo ----------------------------
cmake -S . -B build %GEN% -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo.
    echo [ERROR] Configuration failed! Is a C++ compiler installed?
    pause
    exit /b 1
)
echo [OK] Configuration completed.

echo.
echo Building project
echo ------------------------
cmake --build build --config Release
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [OK] Build completed: build\bin\task01.exe
pause
