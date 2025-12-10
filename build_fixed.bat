@echo off
echo Building FIXED version of ECU Simulator...
cd /d "%~dp0"
cd src
set CC=gcc
set LD=gcc
mingw32-make clean
mingw32-make fixed
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    exit /b 1
)
echo Build successful!
cd ..
echo.
echo FIXED ECU Simulator executable created: src\ecu_sim_fixed.exe
echo This version properly validates VIN input and prevents the vulnerability.
