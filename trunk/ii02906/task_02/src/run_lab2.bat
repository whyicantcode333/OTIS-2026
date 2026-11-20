@echo off
echo ========================================
echo Compiling and running Lab 2 tests
echo ========================================

cd /d "%~dp0"

if not exist build mkdir build
cd build

set CMAKE_POLICY_VERSION_MINIMUM=3.5

cmake -S .. -B . -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug

echo ========================================
echo Running Google Test...
echo ========================================
.\Debug\run_tests.exe

echo ========================================
echo Generating coverage report...
echo ========================================
OpenCppCoverage.exe --sources "D:\OTIS\OTIS-2026\trunk\ii02906\task_02\src" --excluded_sources "D:\OTIS\OTIS-2026\trunk\ii02906\task_02\src\build" -- .\Debug\run_tests.exe

echo ========================================
echo Opening coverage report in browser...
echo ========================================
for /d %%i in (CoverageReport*) do start "" "%%i\index.html"

cd ..
pause