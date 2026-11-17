
@echo off
chcp 65001 >nul
setlocal

echo ========================================
echo    СБОРКА И ТЕСТИРОВАНИЕ TASK_02
echo ========================================

:: Переходим в папку BAT-файла
cd /d "%~dp0"

:: Создаем папку build, если ее нет
if not exist build (
    mkdir build
)

echo.
echo [1/4] Конфигурация проекта (CMake)...

cmake -S . -B build

if errorlevel 1 (
    echo.
    echo ОШИБКА: CMake не смог настроить проект.
    pause
    exit /b 1
)

echo.
echo [2/4] Компиляция проекта...

cmake --build build --config Debug

if errorlevel 1 (
    echo.
    echo ОШИБКА: Ошибка компиляции!
    pause
    exit /b 1
)

echo.
echo [3/4] Запуск Google Test...

ctest --test-dir build -C Debug --output-on-failure > test_report.txt 2>&1

set "TEST_RESULT=%ERRORLEVEL%"

type test_report.txt

if not "%TEST_RESULT%"=="0" (
    echo.
    echo ОШИБКА: Некоторые тесты не пройдены!
    echo Подробности находятся в test_report.txt
    pause
    exit /b 1
)

echo.
echo ВСЕ ТЕСТЫ УСПЕШНО ПРОЙДЕНЫ!


echo.
echo [4/4] Проверка покрытия кода...

set "REPORT_DIR=%~dp0CoverageReport"
set "REPORT_FILE=%REPORT_DIR%\index.html"

where OpenCppCoverage.exe >nul 2>&1

if errorlevel 1 (
    echo [ПРЕДУПРЕЖДЕНИЕ] OpenCppCoverage не найден!
    echo Установите OpenCppCoverage и добавьте его в PATH.
) else (
    if not exist "%REPORT_DIR%" (
        mkdir "%REPORT_DIR%"
    )

    OpenCppCoverage.exe --sources "%~dp0src" --excluded_sources *test* --export_type html:"%REPORT_DIR%" -- "%~dp0build\Debug\task_02_tests.exe"

    if errorlevel 1 (
        echo [ОШИБКА] Не удалось создать отчет покрытия!
    ) else (
        if exist "%REPORT_FILE%" (
            echo.
            echo [OK] Отчет покрытия создан!
            echo Открываем отчет в браузере...

            start "" "%REPORT_FILE%"
        ) else (
            echo [ОШИБКА] Файл index.html не найден!
        )
    )
)

echo.
echo ========================================
echo       РАБОТА ЗАВЕРШЕНА!
echo ========================================
echo Отчет тестирования: test_report.txt
echo Отчет покрытия: CoverageReport\index.html
echo.
