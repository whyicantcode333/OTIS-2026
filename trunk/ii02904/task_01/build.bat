@echo off
cd /d "%~dp0"
chcp 65001 >nul
echo ===== 1. Настройка CMake =====
where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake не найден в PATH. Установите CMake.
    goto error
)
where g++ >nul 2>nul
if errorlevel 1 (
    cmake -S src -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5
) else (
    cmake -S src -B build -G "MinGW Makefiles" -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)
if errorlevel 1 goto error

echo.
echo ===== 2. Компиляция =====
cmake --build build --config Release
if errorlevel 1 goto error

set "EXE=build\task_01.exe"
if not exist "%EXE%" set "EXE=build\Release\task_01.exe"
if not exist "%EXE%" (
    echo Не найден task_01.exe после сборки.
    goto error
)
echo Сборка прошла успешно: %EXE%

echo.
echo ===== 3. Виртуальное окружение Python =====
if not exist ".venv\Scripts\python.exe" (
    set "SYS_PYTHON=python"
    where python >nul 2>nul
    if errorlevel 1 set "SYS_PYTHON=py"
    echo Создаю виртуальное окружение .venv
    %SYS_PYTHON% -m venv .venv
    if errorlevel 1 goto error
)
".venv\Scripts\python.exe" -c "import matplotlib" >nul 2>nul
if errorlevel 1 (
    echo Устанавливаю matplotlib в .venv
    ".venv\Scripts\python.exe" -m pip install -r requirements.txt
    if errorlevel 1 goto error
)
echo Окружение .venv готово

:again
echo.
echo ===== 4. Запуск программы =====
if exist results.csv del results.csv
"%EXE%"
if errorlevel 1 goto error

echo.
echo ===== 5. График =====
".venv\Scripts\python.exe" src\plot.py
if errorlevel 1 goto error

echo.
choice /c YN /m "Запустить программу ещё раз"
if errorlevel 2 goto end
goto again

:end
pause
exit /b 0

:error
echo.
echo ===== ОШИБКА, см. сообщения выше =====
pause
exit /b 1
