@echo off
echo Building ECU Simulator...
cd /d "%~dp0"
cd src
set CC=gcc
set LD=gcc
mingw32-make clean
mingw32-make
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    exit /b 1
)
echo Build successful!
cd ..
echo.
echo ECU Simulator executable created: src\ecu_sim.exe
