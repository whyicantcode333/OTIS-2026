@echo off
REM ============================================================
REM  Сборка проекта и запуск модульных тестов (Google Test)
REM ============================================================

setlocal
cd /d "%~dp0"

echo ============================================
echo  Building project and running tests...
echo ============================================
echo.

REM --- Проверка CMake ---
where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH.
    pause
    exit /b 1
)

REM --- Создание папки сборки ---
if not exist "build" mkdir build
cd build

REM --- Конфигурация ---
echo [1/3] Configuring with CMake...
cmake ..
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

REM --- Сборка ---
echo.
echo [2/3] Building...
cmake --build . --config Debug
if errorlevel 1 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

REM --- Запуск тестов ---
echo.
echo [3/3] Running tests...
echo ============================================
echo  Google Test results
echo ============================================
test\Debug\run_tests.exe
if errorlevel 1 (
    echo.
    echo [FAILED] Some tests failed!
    pause
    exit /b 1
)

echo.
echo ============================================
echo  All tests passed!
echo ============================================

pause
endlocal