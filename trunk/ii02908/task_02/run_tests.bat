@echo off
setlocal

echo ============================================================
echo   BUILDING TESTS (Visual Studio 2022 / MSVC)
echo ============================================================
echo.

set "ROOT=%~dp0"
cd /d "%ROOT%"

echo [1/2] Configuring CMake (Debug)...
cmake -S "%ROOT%." -B "%ROOT%build" -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 (
    echo.
    echo [ERROR] CMake configuration FAILED.
    pause
    exit /b 1
)
echo [OK] CMake configuration finished successfully.
echo.

echo [2/2] Building target 'tests' (Release)...
cmake --build "%ROOT%build" --config Debug --target tests
if errorlevel 1 (
    echo.
    echo [ERROR] Build FAILED.
    pause
    exit /b 1
)
echo.
echo [OK] Build finished successfully.
echo Executable: %ROOT%build\Debug\tests.exe
echo.

pause

:: ==============================================================================
:: НАСТРОЙКИ ПУТЕЙ
:: ==============================================================================

:: Папка, где установлен OpenCppCoverage (по умолчанию)
set "COVERAGE_TOOL=C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe"

:: Папка с исходным кодом, который мы хотим анализировать
set "SOURCE_DIR=%~dp0src"

:: Путь к исполняемому файлу с тестами.
set "TEST_EXE=%~dp0build\Debug\tests.exe"

:: Папка, куда будет сохранен HTML-отчет (создастся автоматически)
set "REPORT_DIR=%~dp0coverage_report"

:: ==============================================================================
:: ЗАПУСК
:: ==============================================================================


echo Running coverage analysis...
echo Analyzing sources in: %SOURCE_DIR%
echo Testing executable: %TEST_EXE%
echo Report will be saved to: %REPORT_DIR%

:: Проверка, существует ли исполняемый файл тестов
if not exist "%TEST_EXE%" (
    echo ERROR: Test executable not found at %TEST_EXE%
    echo Please check the path or build the tests first.
    pause
    exit /b 1
)

:: Запуск OpenCppCoverage
"%COVERAGE_TOOL%" ^
 --sources "%SOURCE_DIR%" ^
 --export_type=html:"%REPORT_DIR%" ^
 -- "%TEST_EXE%"

if %ERRORLEVEL% neq 0 (
    echo Coverage analysis failed or tests returned non-zero exit code.
) else (
    echo Coverage analysis completed successfully.
    echo Opening report...
    start "" "%REPORT_DIR%\index.html"
)

pause


endlocal