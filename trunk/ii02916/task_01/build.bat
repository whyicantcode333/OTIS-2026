@echo off
chcp 65001 > nul
echo ============================================
echo    Сборка проекта OTIS Lab 1 (Variant 15)
echo ============================================

REM Проверяем, есть ли CMakeLists.txt в папке src
if not exist "src\CMakeLists.txt" (
    echo [ERROR] Файл CMakeLists.txt не найден в папке: src\
    pause
    exit /b 1
)

echo === 1. Создание папки build и конфигурация ===
cmake -S src -B build

echo.
echo === 2. Компиляция проекта ===
cmake --build build --config Release

if %errorlevel% neq 0 (
    echo [Ошибка] Компиляция не удалась!
    pause
    exit /b %errorlevel%
)

echo.
echo === 3. Запуск симуляции (C++) ===
REM Ищем .exe файл в возможных папках сборки
if exist build\lab01_ii02916.exe (
    build\lab01_ii02916.exe
) else if exist build\Debug\lab01_ii02916.exe (
    build\Debug\lab01_ii02916.exe
) else if exist build\Release\lab01_ii02916.exe (
    build\Release\lab01_ii02916.exe
) else (
    echo [Ошибка] Не найден скомпилированный exe-файл!
    pause
    exit /b 1
)

echo.
echo === 4. Построение графика (Python) ===
if exist src\plot.py (
    python src\plot.py
) else (
    echo [Предупреждение] Файл src\plot.py не найден, график не построен.
)

echo.
echo === Всё готово! ===
pause
