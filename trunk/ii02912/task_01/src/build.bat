@echo off
chcp 65001 >nul
setlocal
rem Использование: build.bat [n]   (n - количество шагов симуляции, по умолчанию 20)
set N=%1
if "%N%"=="" set N=20

echo ===== [1/4] Конфигурация CMake =====
cmake -S . -B build
if errorlevel 1 goto :error

echo.
echo ===== [2/4] Сборка проекта =====
cmake --build build --config Release
if errorlevel 1 goto :error

echo.
echo ===== [3/4] Запуск программы (n = %N%) =====
if exist build\Release\lab01.exe (set EXE=build\Release\lab01.exe) else (set EXE=build\lab01.exe)
%EXE% %N% results
if errorlevel 1 goto :error

echo.
echo ===== [4/4] Построение графиков (Python/Matplotlib) =====
python plot.py results results\plots.png
if errorlevel 1 echo [!] Графики не построены: установите Python и matplotlib (pip install matplotlib)

echo.
echo ===== Готово =====
pause
exit /b 0

:error
echo.
echo [ОШИБКА] Сборка или запуск завершились с ошибкой.
pause
exit /b 1
