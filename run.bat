@echo off
echo Starting ECU Programming Tool GUI...
cd /d "%~dp0"

if not exist "src\ecu_sim.exe" (
    echo ERROR: ecu_sim.exe not found!
    echo Please run build.bat first to compile the C application.
    pause
    exit /b 1
)

python gui\ecu_gui.py

if %errorlevel% neq 0 (
    echo.
    echo Failed to start GUI. Make sure Python 3 is installed.
    pause
    exit /b 1
)
