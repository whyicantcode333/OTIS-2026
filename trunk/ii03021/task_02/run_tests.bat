@echo off
setlocal
rem UTF-8 console and Python output, so that Russian text is not garbled.
for /f "tokens=2 delims=:" %%a in ('chcp') do set "OLD_CP=%%a"
if defined OLD_CP set "OLD_CP=%OLD_CP: =%"
chcp 65001 >nul
set "PYTHONUTF8=1"
set "PYTHONIOENCODING=utf-8"
cd /d "%~dp0"

set "BUILD_DIR=build"
set "REPORT_DIR=report"
set "TEST_EXE=%BUILD_DIR%\test\otis_tests.exe"
set "EXIT_CODE=0"

echo =======================================
echo  OTIS Lab 2: build, unit tests, coverage
echo =======================================

where cmake >nul 2>nul
if errorlevel 1 (
  echo ERROR: CMake was not found in PATH.
  set "EXIT_CODE=1"
  goto :finish
)

rem Pick a generator that exists on this machine (same logic as task_01/src/build.bat).
where ninja >nul 2>nul
if not errorlevel 1 (
  set "GENERATOR=Ninja"
) else (
  where mingw32-make >nul 2>nul
  if not errorlevel 1 (
    set "GENERATOR=MinGW Makefiles"
  ) else (
    where make >nul 2>nul
    if not errorlevel 1 (
      set "GENERATOR=Unix Makefiles"
    ) else (
      echo ERROR: no build tool found in PATH. Install Ninja or MinGW make.
      set "EXIT_CODE=1"
      goto :finish
    )
  )
)

rem A build directory is tied to its generator and source path: drop it if either changed.
set "SOURCE_DIR=%CD:\=/%"
if exist "%BUILD_DIR%\CMakeCache.txt" (
  findstr /b /c:"CMAKE_GENERATOR:INTERNAL=%GENERATOR%" "%BUILD_DIR%\CMakeCache.txt" >nul
  if errorlevel 1 (
    rmdir /s /q "%BUILD_DIR%"
  ) else (
    findstr /b /c:"CMAKE_HOME_DIRECTORY:INTERNAL=%SOURCE_DIR%" "%BUILD_DIR%\CMakeCache.txt" >nul
    if errorlevel 1 rmdir /s /q "%BUILD_DIR%"
  )
)

if not exist "%REPORT_DIR%" mkdir "%REPORT_DIR%"

echo.
echo [1/4] Configuring, generator: %GENERATOR%
echo       Google Test is downloaded on the first run, internet is required.
cmake -S . -B "%BUILD_DIR%" -G "%GENERATOR%" -DCMAKE_BUILD_TYPE=Debug -DOTIS_COVERAGE=ON
if errorlevel 1 (
  set "EXIT_CODE=1"
  goto :finish
)

echo.
echo [2/4] Building
cmake --build "%BUILD_DIR%"
if errorlevel 1 (
  set "EXIT_CODE=1"
  goto :finish
)

echo.
echo [3/4] Running Google Test through CTest
rem Remove counters of previous runs so that coverage reflects this run only.
del /s /q "%BUILD_DIR%\*.gcda" >nul 2>nul
pushd "%BUILD_DIR%"
ctest --output-on-failure
if errorlevel 1 set "EXIT_CODE=1"
popd

rem Test report: full console log and XML (one record per test) for the readme.
"%TEST_EXE%" --gtest_color=no --gtest_output=xml:"%REPORT_DIR%\gtest_report.xml" > "%REPORT_DIR%\gtest_report.txt" 2>&1
if errorlevel 1 set "EXIT_CODE=1"
echo Test report: %REPORT_DIR%\gtest_report.txt and %REPORT_DIR%\gtest_report.xml

echo.
echo [4/4] Code coverage with gcovr
set "GCOVR="
where gcovr >nul 2>nul
if not errorlevel 1 set "GCOVR=gcovr"
if not defined GCOVR (
  python -m gcovr --version >nul 2>nul
  if not errorlevel 1 set "GCOVR=python -m gcovr"
)
if not defined GCOVR (
  echo WARNING: gcovr was not found. Install it: python -m pip install gcovr
  goto :finish
)

rem Root is trunk\ii03020, so that the lab 1 sources compiled from ..\task_01\src are covered too.
rem Excluded: the interactive main.cpp, the tests themselves and downloaded Google Test sources.
%GCOVR% -r .. --object-directory "%CD%\%BUILD_DIR%" --exclude ".*_deps.*" --exclude ".*main\.cpp" --exclude ".*[\\/]test[\\/].*" --exclude ".*[\\/]build[\\/].*" --print-summary --txt "%REPORT_DIR%\coverage.txt" --html-details "%REPORT_DIR%\coverage.html" --json-summary "%REPORT_DIR%\coverage_summary.json" --json-summary-pretty
if errorlevel 1 (
  echo WARNING: gcovr failed.
  goto :finish
)
type "%REPORT_DIR%\coverage.txt"

where python >nul 2>nul
if not errorlevel 1 (
  python tools\coverage_report.py "%REPORT_DIR%\coverage_summary.json" "%REPORT_DIR%\gtest_report.xml" "%REPORT_DIR%\coverage_summary.html" > "%REPORT_DIR%\readme_section.md"
  echo.
  type "%REPORT_DIR%\readme_section.md"
)
echo.
echo HTML summary, lines and functions: %REPORT_DIR%\coverage_summary.html
echo HTML detailed report: %REPORT_DIR%\coverage.html

:finish
echo.
if "%EXIT_CODE%"=="0" (
  echo RESULT: SUCCESS
) else (
  echo RESULT: FAILURE, see messages above
)
if /i not "%~1"=="nopause" pause
if defined OLD_CP chcp %OLD_CP% >nul
exit /b %EXIT_CODE%
