# Motorcycle ECU Programming Tool - Demo

This is a demonstration project simulating a motorcycle ECU (Engine Control Unit) programming tool. It consists of a C console application that simulates an ECU and a Python GUI that communicates with it.

## Purpose

This project demonstrates:
- Embedded C programming for automotive ECU simulation
- Python GUI development with Tkinter
- Inter-process communication via stdin/stdout
- **Intentional security vulnerability** for training purposes (input validation bug in license grade checking)
- CERT C coding standard violations for static analysis training

## Project Structure

```
C:\CERTC-AI-demo\
├── src/
│   ├── ecu_sim.c          # C console ECU simulator with intentional bug
│   ├── Makefile           # Build configuration for MinGW GCC
│   └── ecu_sim.exe        # Compiled executable (after build)
├── gui/
│   └── ecu_gui.py         # Python Tkinter GUI application
├── eclipse/               # Eclipse CDT workspace directory
├── build.bat              # Automated build script
├── run.bat                # Automated run script
└── README.md              # This file
```

## Requirements

### For C Application:
- MinGW GCC (must be in system PATH)
- Make utility (mingw32-make)

### For Python GUI:
- Python 3.x with Tkinter (included in standard Python installation)

## Build Instructions

### Using Automation Script (Recommended):
```batch
build.bat
```

### Manual Build:
```batch
cd src
mingw32-make
cd ..
```

## Running the Application

### Using Automation Script (Recommended):
```batch
run.bat
```

### Manual Run:
```batch
python gui\ecu_gui.py
```

## Usage

1. **Build the application** using `build.bat`
2. **Run the GUI** using `run.bat`
3. **Connect to a motorcycle**: Click "Connect and Identify Motorcycle"
   - The system will randomly select a VIN from the database
   - VIN, ECU type (ROAD/RACE), and allowed maps will be displayed
4. **Select a map** from the dropdown menu
5. **Flash the map**: Click "Flash Selected Map"
   - You will be prompted for your technician license grade (1-3)
   - Enter a valid grade or try malformed input to trigger the bug

## ECU Types and Maps

### ROAD ECU (Street Legal)
- Maximum allowed: 54 HP
- Legal maps: R_A, R_B, R_C
- **Regulatory constraint**: Race maps (X_A, X_B, X_C) are illegal

### RACE ECU (Track Use)
- No power limitation
- All maps allowed: R_A, R_B, R_C, X_A, X_B, X_C

### Available Maps
- **R_A**: Road A - basic (54 HP)
- **R_B**: Road B - intermediate (54 HP)
- **R_C**: Road C - advanced (54 HP)
- **X_A**: Race A (65 HP) - RACE only
- **X_B**: Race B (77 HP) - RACE only
- **X_C**: Race C (100 HP) - RACE only

### VIN Database
- `123456`: ROAD ECU, min license grade 1, allows R_A/R_B/R_C
- `234567`: ROAD ECU, min license grade 2, allows R_A/R_B
- `345678`: RACE ECU, min license grade 3, allows R_A/R_B/R_C

## Intentional Security Vulnerability

### Location
File: `src/ecu_sim.c`, function `flash_map()`, lines ~195-230

### Description
The code uses `scanf("%d", &license_grade)` to read the technician's license grade but **ignores the return value**. When the user enters malformed input (e.g., "2.5", "A3", empty string), scanf fails and leaves `license_grade` at its previous value.

### Impact
Due to the global variable `int license_grade = 3;` being initialized to the highest grade:
1. When scanf fails on malformed input, `license_grade` remains at 3
2. The authorization check passes with the highest privileges
3. **Race maps can be flashed onto ROAD ECUs**, violating regulatory constraints
4. A ROAD ECU with a race map would exceed 54 HP legal limit (up to 100 HP)

### Triggering the Bug
1. Identify a ROAD ECU (VIN 123456 or 234567)
2. Select a race map (X_A, X_B, or X_C)
3. When prompted for license grade, enter malformed input: `2.5` or `A3` or just press Enter
4. The system will flash the race map despite it being illegal for ROAD ECUs

### Related CERT C Violations
- **ERR33-C**: Detect and handle standard library errors (ignored scanf return value)
- **EXP33-C**: Do not read uninitialized memory (stale license_grade value)
- Additional minor violations (strcpy usage, missing const, etc.)

## For Static Analysis

This code is designed to be analyzed with CERT C-compliant static analysis tools. The intentional bug and additional violations provide training material for:
- Understanding critical vs. non-critical violations
- Prioritizing security issues by business impact
- Demonstrating AI-assisted code review and remediation

## Eclipse CDT Integration

To import into Eclipse:
1. Open Eclipse CDT
2. File → Import → General → Existing Projects into Workspace
3. Select root directory: `C:\CERTC-AI-demo`
4. The workspace is configured in the `eclipse/` subdirectory

## License

This is a demonstration/training project. Use for educational purposes only.
