@echo off
setlocal
cd /d "%~dp0"

echo =======================================
echo  Compiling project and running tests...
echo =======================================

if not exist build mkdir build
cd build

cmake .. -G "Ninja" > nul
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

cmake --build .
if errorlevel 1 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo =======================================
echo  Running Google Test...
echo =======================================
.\run_tests.exe

echo.
echo =======================================
echo  Generating Code Coverage...
echo =======================================
gcovr -r .. --exclude ".*_deps.*" --exclude ".*main\.cpp.*" --txt=-

gcovr -r .. --exclude ".*_deps.*" --exclude ".*main\.cpp.*" --html-details -o coverage.html
echo [OK] Coverage report saved to: build\coverage.html

start chrome "file:///%CD%/coverage.html"

cd ..
pause
endlocal