@echo off
setlocal
cd /d "%~dp0"

echo =======================================
echo  Configuring and building project...
echo =======================================
cmake -S . -B build
if errorlevel 1 goto error

cmake --build build --config Release
if errorlevel 1 goto error

echo.
echo =======================================
echo  Running Google Test...
echo =======================================
if exist "build\Release\unit_tests.exe" (
    set "TEST_EXE=build\Release\unit_tests.exe"
) else (
    set "TEST_EXE=build\unit_tests.exe"
)

"%TEST_EXE%" --gtest_output=xml:test_results.xml
set "TEST_STATUS=%errorlevel%"

where gcovr >nul 2>nul
if %errorlevel%==0 (
    echo Generating coverage metrics via gcovr...
    gcovr -r src --json coverage.json
)

echo.
echo =======================================
echo  Generating HTML report...
echo =======================================
python generate_html_report.py test_results.xml coverage.json test_report.html

if exist "test_report.html" (
    echo Opening test_report.html...
    start "" "%~dp0test_report.html"
)

if not "%TEST_STATUS%"=="0" (
    echo.
    echo Some unit tests failed!
    pause
    exit /b %TEST_STATUS%
)

echo.
echo All tests finished successfully!
pause
exit /b 0

:error
echo.
echo Build failed! Check compiler messages above.
pause
exit /b 1