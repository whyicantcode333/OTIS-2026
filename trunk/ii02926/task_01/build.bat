@echo off
setlocal
cd /d "%~dp0"

echo  Building ii02926 / task_01
echo.

REM --- 1. Проверка CMake в PATH (или подключение Visual Studio 2022) ---
where cmake >nul 2>nul
if errorlevel 1 (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
        call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" (
        call "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" (
        call "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
    )
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake not found. Please install CMake or configure PATH.
    pause
    exit /b 1
)

REM --- 2. Сборка C++ проекта через CMake ---
if not exist "build" mkdir build
cd build

echo [1/4] Configuring with CMake...
cmake ..
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    cd ..
    pause
    exit /b 1
)

echo.
echo [2/4] Building Release...
cmake --build . --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    cd ..
    pause
    exit /b 1
)

cd ..

REM --- 3. Удаляем старый CSV и запускаем нашу программу ---
echo.
echo [3/4] Running simulation...
if exist "result.csv" del /q "result.csv"

if exist "build\src\Release\ii02926_task_01.exe" (
    "build\src\Release\ii02926_task_01.exe"
) else if exist "build\src\ii02926_task_01.exe" (
    "build\src\ii02926_task_01.exe"
) else (
    echo [ERROR] Executable ii02926_task_01.exe not found!
    pause
    exit /b 1
)

if not exist "result.csv" (
    echo [WARN] Simulation cancelled or result.csv was not created.
    pause
    exit /b 1
)

REM --- 4. Запускаем Python для показа графика ---
echo.
echo [4/4] Opening plot...
python plot_results.py
if errorlevel 1 (
    py plot_results.py
)

echo.
echo  Done!
pause

endlocal