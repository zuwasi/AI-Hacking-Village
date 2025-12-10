@echo off
echo Starting FIXED ECU Programming Tool GUI...
cd /d "%~dp0"

if not exist "src\ecu_sim_fixed.exe" (
    echo ERROR: ecu_sim_fixed.exe not found!
    echo Please run build_fixed.bat first to compile the FIXED version.
    pause
    exit /b 1
)

echo Using FIXED version (src\ecu_sim_fixed.exe)
echo This version has proper input validation.
echo.

REM Temporarily rename files to use fixed version
if exist "src\ecu_sim.exe.bak" del "src\ecu_sim.exe.bak"
if exist "src\ecu_sim.exe" ren "src\ecu_sim.exe" "ecu_sim.exe.bak"
copy "src\ecu_sim_fixed.exe" "src\ecu_sim.exe" >nul

python gui\ecu_gui.py

REM Restore original
if exist "src\ecu_sim.exe" del "src\ecu_sim.exe"
if exist "src\ecu_sim.exe.bak" ren "src\ecu_sim.exe.bak" "ecu_sim.exe"

if %errorlevel% neq 0 (
    echo.
    echo Failed to start GUI. Make sure Python 3 is installed.
    pause
    exit /b 1
)
