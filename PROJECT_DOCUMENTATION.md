# ECU Demo Project - Complete Documentation

## Project Overview

This project was created as a demonstration/training tool for CERT C static analysis and security vulnerability detection. It simulates a motorcycle ECU (Engine Control Unit) programming system with an intentional security vulnerability.

**Created:** December 6, 2025  
**Location:** `C:\CERTC-AI-demo`  
**Repository:** https://github.com/zuwasi/My_CUDA_Profiler

---

## Project Structure

```
C:\CERTC-AI-demo\
├── src/
│   ├── ecu_sim.c              # C ECU simulator with intentional vulnerability
│   ├── Makefile               # Build configuration for MinGW GCC
│   ├── ecu_sim.o              # Compiled object file (generated)
│   └── ecu_sim.exe            # Executable (generated)
├── gui/
│   └── ecu_gui.py             # Python Tkinter GUI application
├── eclipse/                   # Eclipse CDT workspace directory
├── build.bat                  # Automated build script
├── run.bat                    # Automated GUI launch script
├── .gitignore                 # Git ignore patterns
├── README.md                  # User documentation
├── implement ECU demo.md      # Implementation specification
├── Motorcycle ECU Demo – C Console A.md  # Detailed requirements spec
└── PROJECT_DOCUMENTATION.md   # This file
```

---

## Implementation Timeline

### Phase 1: Initial Setup
1. Created git repository: `C:\CERTC-AI-demo`
2. Initialized as git repository: `git init`
3. Created directory structure:
   - `src/` - C source code
   - `gui/` - Python GUI
   - `eclipse/` - Eclipse CDT workspace

### Phase 2: C Application Development
Created `src/ecu_sim.c` with:
- VIN database (3 motorcycles: 2 ROAD, 1 RACE)
- 6 ignition maps (3 ROAD ≤54HP, 3 RACE up to 100HP)
- Command-line protocol (IDENTIFY, FLASH_MAP, QUIT)
- **Intentional scanf vulnerability** for VIN verification

### Phase 3: Build System
- Created `src/Makefile` for MinGW GCC
- Supports variable overrides for Parasoft scanner: `CC`, `LD`
- Separate compile and link steps for static analysis compatibility
- Created `build.bat` automation script with path fixing

### Phase 4: Python GUI Development
Created `gui/ecu_gui.py` with Tkinter:
- Vehicle identification button
- Map selection dropdown
- Flash map button with warnings
- **Verify Flash Process** button for compliance checking
- Real-time log display
- Subprocess communication with C executable via stdin/stdout

### Phase 5: Enhanced Features
Added verification and warning systems:
- Red warning label when attempting illegal race map flash
- Verification button to detect ROAD ECU + race map violations
- Dialog with option to ignore regulatory violation
- Color-coded verification results (green/red/blue)

### Phase 6: Eclipse CDT Integration
- Configured Eclipse project for MinGW GCC
- Set up Parasoft C/C++test integration
- Fixed build command to use `mingw32-make`
- Configured Parasoft Build Settings for static analysis

---

## Technical Implementation Details

### C Application Architecture

**Global State:**
```c
char current_vin[MAX_VIN_LEN] = "";       // Currently identified VIN
ECUType current_ecu_type;                  // ROAD or RACE
int current_min_license = 0;               // Minimum license required
int vin_verification = -1;                 // VIN verification (BUG HERE!)
```

**VIN Database:**
```c
VehicleInfo vehicles[MAX_VINS] = {
    {"123456", ECU_ROAD, 1, {"R_A", "R_B", "R_C"}, 3},  // ROAD ECU
    {"234567", ECU_ROAD, 2, {"R_A", "R_B"}, 2},         // ROAD ECU
    {"345678", ECU_RACE, 3, {"R_A", "R_B", "R_C"}, 3}   // RACE ECU
};
```

**Ignition Maps:**
- **R_A**: Road A - basic (54 HP) - Legal on ROAD
- **R_B**: Road B - intermediate (54 HP) - Legal on ROAD
- **R_C**: Road C - advanced (54 HP) - Legal on ROAD
- **X_A**: Race A (65 HP) - **ILLEGAL on ROAD**
- **X_B**: Race B (77 HP) - **ILLEGAL on ROAD**
- **X_C**: Race C (100 HP) - **ILLEGAL on ROAD**

**Command Protocol:**
- `IDENTIFY` - Randomly selects and identifies a VIN
- `FLASH_MAP <map_id>` - Flashes selected map (triggers VIN verification)
- `QUIT` - Exits the program

### Python GUI Architecture

**Main Components:**
1. **Vehicle Information Panel**
   - VIN display
   - ECU type display
   - Connect/Identify button

2. **Map Selection Panel**
   - Dropdown with all 6 maps
   - Flash button
   - Warning label (red, bold)

3. **Verification Panel**
   - Verify Flash Process button
   - Result label (color-coded)

4. **Log Panel**
   - Scrollable text area
   - Real-time command/response logging

**Subprocess Communication:**
- Launches `src/ecu_sim.exe` as subprocess
- stdin: sends commands
- stdout: reads responses
- Background thread for non-blocking I/O
- Queue-based message handling

---

## The Intentional Vulnerability

### Location
**File:** `src/ecu_sim.c`  
**Function:** `flash_map()`  
**Lines:** ~177-194

### The Bug

```c
printf("PROMPT: Re-enter VIN from motorcycle documents for verification:\n");
fflush(stdout);

scanf("%d", &vin_verification);  // ❌ RETURN VALUE IGNORED!

int current_vin_num = atoi(current_vin);

if (vin_verification == current_vin_num) {
    if (current_ecu_type == ECU_ROAD && map->is_race_map) {
        printf("ERROR: Race maps not allowed on ROAD ECUs (regulatory violation)\n");
        fflush(stdout);
        return;
    }
} else if (vin_verification >= 100000 && vin_verification <= 999999) {
    printf("ERROR: VIN mismatch. Please try again.\n");
    fflush(stdout);
    return;
}
// ❌ If vin_verification is outside valid range, ALL checks are skipped!
```

### Why It's Vulnerable

1. **Global variable initialized to -1:**
   ```c
   int vin_verification = -1;
   ```

2. **scanf() return value is not checked:**
   - Violates **CERT C ERR33-C**: "Detect and handle standard library errors"
   
3. **When user enters malformed input:**
   - `scanf("%d", &vin_verification)` returns 0 (failure)
   - `vin_verification` remains at `-1` (unmodified)
   - Violates **CERT C EXP33-C**: "Do not read uninitialized memory"

4. **Logic bypass:**
   - `-1 != current_vin_num` (not equal check passes)
   - `-1 < 100000` (not in valid VIN range)
   - Both `if` and `else if` conditions fail
   - **All security checks are skipped!**

### Impact

**Regulatory Violation:**
- ROAD ECUs have a **54 HP legal limit** for street-legal motorcycles
- Race maps can produce **65-100 HP**
- Flashing a race map to a ROAD ECU violates street-legal regulations
- Motorcycle becomes illegal for road use

**Security Impact:**
- Authorization bypass
- Integrity violation (incorrect ECU configuration)
- Safety risk (unexpected power output)

---

## How to Trigger the Vulnerability

### Normal Operation (Security Works)

**Scenario 1: Correct VIN Entry**
1. Identify ROAD ECU (VIN: 123456)
2. Select race map X_C
3. Enter VIN correctly: `123456`
4. **Result:** ✅ "ERROR: Race maps not allowed on ROAD ECUs"

**Scenario 2: Wrong but Valid VIN**
1. Identify ROAD ECU (VIN: 123456)
2. Select race map X_C
3. Enter different valid VIN: `234567`
4. **Result:** ✅ "ERROR: VIN mismatch. Please try again."

### Backdoor Activation (Vulnerability Exploited)

**Malformed Input Types:**
- `123.5` (decimal/float)
- `ABC` (letters)
- `x3` or `a123` (starts with letter)
- Empty input (just press Enter/OK)
- Whitespace only

**Step-by-Step Exploit:**
1. Run GUI: `run.bat`
2. Click **"Connect and Identify Motorcycle"** until you get a ROAD ECU
   - VIN: `123456` or `234567`
   - ECU Type: `ROAD`
3. Select a race map from dropdown:
   - `X_A (Race A, 65HP)` or
   - `X_B (Race B, 77HP)` or
   - `X_C (Race C, 100HP)`
4. Red warning appears: **"⚠️ WARNING: You are trying to flash an ILLEGAL RACE MAP to a ROAD ECU! ⚠️"**
5. Click **"Flash Selected Map"**
6. VIN verification dialog appears
7. **Enter malformed input:** type `ABC` or `123.5` or just click OK without typing
8. **Result:** ⚠️ "SUCCESS: Map 'X_C' (Race C - 100 HP, 100HP) flashed to VIN 123456"
9. Click **"Verify Flash Process"**
10. **Violation detected:** Red alert dialog with option to ignore

**Expected Log Output:**
```
⚠️ REGULATORY WARNING: Attempting to flash illegal race map to ROAD ECU
Initiating flash for map: X_C
WARNING: Flashing race map (100HP) on ROAD ECU may violate regulations!
SUCCESS: Map 'X_C' (Race C - 100 HP, 100HP) flashed to VIN 123456
```

---

## CERT C Violations

### Critical Violations (Security-Relevant)

**ERR33-C: Detect and handle standard library errors**
- Location: `scanf("%d", &vin_verification);`
- Issue: Return value ignored
- Impact: Authorization bypass

**EXP33-C: Do not read uninitialized memory**
- Location: Use of `vin_verification` after failed scanf
- Issue: Stale/uninitialized value used in security decision
- Impact: Logic bypass

### Non-Critical Violations (Training Noise)

**STR07-C: Use TR 24731 Bounds-checking interfaces**
- Location: `strcpy(current_vin, vehicle->vin);`
- Issue: strcpy instead of strcpy_s/strncpy
- Impact: Minimal (buffer is sized appropriately)

**DCL00-C: Const-qualify immutable objects**
- Location: Various function parameters
- Issue: Missing const qualifiers on read-only parameters
- Impact: None (code quality issue only)

**ERR33-C: Detect and handle standard library errors** (minor)
- Location: Multiple `printf()` calls
- Issue: Return value ignored
- Impact: None (output failure not critical for demo)

---

## Build and Run Instructions

### Prerequisites
- **MinGW GCC** (must be in system PATH)
- **mingw32-make** utility
- **Python 3.x** with Tkinter (standard library)
- **Eclipse CDT** (optional, for IDE integration)

### Build from Command Line

```batch
cd C:\CERTC-AI-demo
build.bat
```

**What it does:**
1. Changes to script's directory
2. Sets `CC=gcc` and `LD=gcc`
3. Runs `mingw32-make clean`
4. Runs `mingw32-make`
5. Creates `src\ecu_sim.exe`

### Run the Application

```batch
cd C:\CERTC-AI-demo
run.bat
```

**What it does:**
1. Changes to script's directory
2. Checks if `src\ecu_sim.exe` exists
3. Launches `python gui\ecu_gui.py`
4. GUI automatically starts the C subprocess

### Build from Eclipse CDT

**Setup:**
1. File → New → C Project
2. Project name: `ecu_simulator`
3. Location: `C:\CERTC-AI-demo\src` (uncheck "Use default")
4. Project Type: Makefile Project → Empty Project
5. Toolchain: MinGW GCC

**Configure Builder:**
1. Right-click project → Properties → Builders
2. Uncheck "CDT Core Builder"
3. New... → Program
4. Name: `MinGW Make Build`
5. Location: `C:\CERTC-AI-demo\build.bat`
6. Working Directory: `C:\CERTC-AI-demo`

**Build:**
- Project → Build Project
- Or click the hammer icon

### Parasoft C/C++test Integration

**Configure Build Settings:**
1. Right-click project → Properties
2. Parasoft → C/C++test → Build Settings
3. Build command line: `mingw32-make -i -B CXX=${CPPTEST_SCAN} CC=${CPPTEST_SCAN} LD=${CPPTEST_SCAN}`
4. Build working directory: `${project_loc}`
5. Dependency file(s): `${project_loc}/Makefile`

**Run Static Analysis:**
1. Right-click project → Parasoft → Test Configurations
2. Select "SEI CERT C Guidelines" or custom configuration
3. Click "Test" button
4. Review violations in Parasoft Console/Report

---

## Testing Scenarios

### Test Case 1: Normal ROAD Map Flash (Should Succeed)

**Steps:**
1. Identify ROAD ECU
2. Select R_A or R_B or R_C
3. Enter correct VIN
4. **Expected:** SUCCESS message, map flashed
5. Verify → **Expected:** Green "Configuration valid"

### Test Case 2: Correct VIN + Race Map (Should Fail)

**Steps:**
1. Identify ROAD ECU (e.g., 123456)
2. Select X_C
3. Enter correct VIN: `123456`
4. **Expected:** ERROR "Race maps not allowed on ROAD ECUs"
5. Verify → **Expected:** No map flashed

### Test Case 3: Wrong VIN Entry (Should Fail)

**Steps:**
1. Identify ROAD ECU (e.g., 123456)
2. Select any map
3. Enter different valid VIN: `234567`
4. **Expected:** ERROR "VIN mismatch. Please try again."

### Test Case 4: Vulnerability Exploit (Should Succeed Incorrectly)

**Steps:**
1. Identify ROAD ECU (e.g., 123456)
2. Select X_C race map
3. Red warning appears
4. Click Flash
5. Enter malformed input: `ABC`
6. **Expected:** ⚠️ WARNING + SUCCESS (vulnerability triggered!)
7. Verify → **Expected:** Red "VIOLATION DETECTED"
8. **Expected Dialog:** Regulatory violation with ignore option

### Test Case 5: RACE ECU (All Maps Allowed)

**Steps:**
1. Identify RACE ECU (345678)
2. Select any map (R_A through X_C)
3. Enter correct VIN: `345678`
4. **Expected:** SUCCESS for all maps
5. Verify → **Expected:** Green "Configuration valid"

### Test Case 6: Multiple Vulnerability Triggers

**Different malformed inputs to test:**
- `123.5` (decimal)
- `ABC` (letters)
- `-123` (negative, though scanf might read it)
- Empty (just press Enter)
- `x3` (starts with letter)
- ` ` (whitespace)

**All should trigger the bypass!**

---

## Development Decisions and Iterations

### Iteration 1: License Grade Authentication (Discarded)
- **Original Idea:** Technician enters license grade (1-3)
- **Problem:** Not operationally realistic
- **User Feedback:** "doesn't make operational sense"

### Iteration 2: VIN Verification (Final Implementation)
- **Improved Design:** Re-enter VIN from motorcycle documents
- **Operational Flow:** Realistic mechanic workflow
- **Vulnerability:** Malformed VIN input bypasses all checks
- **Makes Sense:** Technician would verify VIN from paperwork

### Challenge: Global Variable Initialization
- **First attempt:** `int vin_verification = 3;` (for license grade)
- **Issue with 999999:** Still in valid VIN range (100000-999999)
- **Final solution:** `int vin_verification = -1;` (outside valid range)

### GUI Enhancement Requests
1. **"Verify Flash Process" button** - detects mismatches
2. **Colorful warnings** - red for illegal attempts
3. **Ignore option** - realistic choice when violation detected
4. **Clear visual feedback** - users know what's happening

### Eclipse/Parasoft Integration Challenges
- **Issue:** Default builder looks for `make`, but MinGW uses `mingw32-make`
- **Solution:** Override CC and LD in build.bat, configure Parasoft build command
- **Issue:** Makefile needs variable override support for Parasoft
- **Solution:** Use `CC ?= gcc` and `LD ?= $(CC)` in Makefile

---

## Files Created/Modified

### New Files Created
1. `C:\CERTC-AI-demo\src\ecu_sim.c` - C ECU simulator
2. `C:\CERTC-AI-demo\src\Makefile` - Build configuration
3. `C:\CERTC-AI-demo\gui\ecu_gui.py` - Python GUI
4. `C:\CERTC-AI-demo\build.bat` - Build automation
5. `C:\CERTC-AI-demo\run.bat` - Run automation
6. `C:\CERTC-AI-demo\.gitignore` - Git ignore patterns
7. `C:\CERTC-AI-demo\README.md` - User documentation
8. `C:\CERTC-AI-demo\PROJECT_DOCUMENTATION.md` - This file

### Directories Created
1. `C:\CERTC-AI-demo\src\` - C source code
2. `C:\CERTC-AI-demo\gui\` - Python GUI
3. `C:\CERTC-AI-demo\eclipse\` - Eclipse workspace

### Existing Files (Reference)
1. `C:\CERTC-AI-demo\implement ECU demo.md` - Implementation spec
2. `C:\CERTC-AI-demo\Motorcycle ECU Demo – C Console A.md` - Requirements

---

## Key Achievements

✅ **Fully functional ECU simulator** with realistic business logic  
✅ **Intentional, plausible vulnerability** for training purposes  
✅ **Professional GUI** with warnings and verification  
✅ **Automated build system** compatible with Eclipse and Parasoft  
✅ **Complete documentation** for users and developers  
✅ **CERT C violations** both critical and non-critical for training  
✅ **Realistic operational workflow** that makes sense to mechanics  
✅ **Git repository** ready for version control  

---

## Future Enhancements (Optional)

1. **Additional VINs:** Expand the database
2. **Logging to file:** Persist flash history
3. **Configuration file:** External VIN/map database
4. **Unit tests:** Automated testing of vulnerability
5. **Fix implementation:** Demonstrate proper scanf error handling
6. **Network mode:** Simulate remote ECU programming
7. **Authentication:** Multi-factor technician verification

---

## Conclusion

This project successfully demonstrates:
- A realistic embedded C application with a subtle security vulnerability
- The importance of proper input validation and error handling
- CERT C coding standard violations and their security implications
- Integration with professional static analysis tools (Parasoft)
- Practical training material for AI-assisted code review and remediation

The intentional bug (ignoring scanf return value) is a common real-world vulnerability that illustrates why CERT C guidelines exist and why automated static analysis is critical for safety-critical software.

---

**End of Documentation**
