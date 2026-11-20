@echo off
cd /d "%~dp0"

echo ========================================
echo          RUNNING ALL TESTS
echo ========================================

call run_tests.bat

echo.
echo ========================================
echo       STARTING CODE COVERAGE
echo ========================================

call run_coverage.bat

pause