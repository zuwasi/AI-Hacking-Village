@echo off
echo Searching for ecu_sim.exe processes...

tasklist /FI "IMAGENAME eq ecu_sim.exe" 2>NUL | find /I /N "ecu_sim.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo Found ecu_sim.exe processes. Killing them...
    taskkill /F /IM ecu_sim.exe
    echo Done!
) else (
    echo No ecu_sim.exe processes found.
)

pause
