@echo off
echo Building BOTH versions of ECU Simulator...
cd /d "%~dp0"
cd src
set CC=gcc
set LD=gcc
mingw32-make clean
mingw32-make both
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    exit /b 1
)
echo Build successful!
cd ..
echo.
echo ==========================================
echo Created TWO executables:
echo ==========================================
echo 1. src\ecu_sim.exe       - VULNERABLE version (intentional bug)
echo 2. src\ecu_sim_fixed.exe - FIXED version (secure)
echo.
echo Use build.bat or build_fixed.bat to build individual versions.
