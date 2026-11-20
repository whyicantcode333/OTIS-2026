@echo off
chcp 65001 > nul
echo ==============================================
echo STARTING BUILD PROCESS (Variant 23)
echo ==============================================

:: Шаг 1: Конфигурация CMake
echo [STAGE 1] CMake Configuration...
cmake -B build -S .
if %errorlevel% neq 0 goto error

:: Шаг 2: Сборка проекта
echo [STAGE 2] Building Project...
cmake --build build --config Release
if %errorlevel% neq 0 goto error

:: Шаг 3: Поиск и запуск исполняемого файла
echo [STAGE 3] Execution
if exist "build\src\Release\task_01.exe" (
    "build\src\Release\task_01.exe"
) else if exist "build\src\Debug\task_01.exe" (
    "build\src\Debug\task_01.exe"
) else if exist "build\src\task_01.exe" (
    "build\src\task_01.exe"
) else if exist "build\src\ii02921_task_01.exe" (
    "build\src\ii02921_task_01.exe"
) else if exist "build\Release\task_01.exe" (
    "build\Release\task_01.exe"
) else if exist "build\Debug\task_01.exe" (
    "build\Debug\task_01.exe"
) else if exist "build\task_01.exe" (
    "build\task_01.exe"
) else if exist "build\ii02921_task_01.exe" (
    "build\ii02921_task_01.exe"
) else (
    echo [ERROR] Executable not found in build directory!
    goto error
)
if %errorlevel% neq 0 goto error

:: Шаг 4: Запуск Python-скрипта для генерации графиков
echo ==============================================
echo [STAGE 4] Generating Plots...
echo ==============================================
if exist "src\plot.py" (
    python src\plot.py
) else if exist "plot.py" (
    python plot.py
) else (
    echo [WARNING] plot.py not found, skipping plot generation.
)

goto end

:error
echo ==============================================
echo ERROR: Process terminated with failure.
echo ==============================================

:end
pause