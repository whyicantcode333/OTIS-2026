@echo off
cd /d "%~dp0"
pushd build\bin
task01.exe
popd
pause
