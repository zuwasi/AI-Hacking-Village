# Testing Guide: Vulnerable vs Fixed Versions

## Overview

The ECU simulator now has **two versions** that can be compiled from the same source code using conditional compilation:

1. **VULNERABLE version** (default) - Contains the intentional scanf bug
2. **FIXED version** (`#ifdef FIXED`) - Properly validates input

---

## Build Commands

### Build Vulnerable Version (Default)
```batch
build.bat
```
Creates: `src\ecu_sim.exe` (vulnerable)

### Build Fixed Version
```batch
build_fixed.bat
```
Creates: `src\ecu_sim_fixed.exe` (secure)

### Build Both Versions
```batch
build_both.bat
```
Creates both executables for comparison testing

### Manual Build Commands
```batch
cd src

# Vulnerable version
mingw32-make

# Fixed version
mingw32-make fixed

# Both versions
mingw32-make both

# Clean all
mingw32-make clean
```

---

## Run Commands

### Run Vulnerable Version
```batch
run.bat
```
Uses `src\ecu_sim.exe` (default vulnerable version)

### Run Fixed Version
```batch
run_fixed.bat
```
Temporarily swaps in the fixed executable

---

## Testing Scenarios

### Test 1: Normal Operation (Both Should Work)

**Steps:**
1. Launch GUI
2. Click "Connect and Identify Motorcycle"
3. Get ROAD ECU (VIN 123456 or 234567)
4. Select road map (R_A, R_B, or R_C)
5. Click "Flash Selected Map"
6. Enter correct VIN: `123456`

**Expected Result (Both Versions):**
- ✅ SUCCESS: Map flashed

---

### Test 2: Wrong VIN Entry (Both Should Block)

**Steps:**
1. Connect to ROAD ECU (VIN 123456)
2. Select any map
3. Enter wrong VIN: `234567`

**Expected Result (Both Versions):**
- ✅ ERROR: VIN mismatch. Please try again.

---

### Test 3: Race Map on ROAD ECU with Correct VIN (Both Should Block)

**Steps:**
1. Connect to ROAD ECU (VIN 123456)
2. Select race map (X_A, X_B, or X_C)
3. Enter correct VIN: `123456`

**Expected Result (Both Versions):**
- ✅ ERROR: Race maps not allowed on ROAD ECUs (regulatory violation)
- ✅ Log window flashes red

---

### Test 4: **THE VULNERABILITY** - Malformed VIN Input

#### Test 4a: Letters
**Steps:**
1. Connect to ROAD ECU (VIN 123456)
2. Select race map X_C
3. Enter malformed VIN: `ABC`

**Expected Results:**

| Version | Result |
|---------|--------|
| **Vulnerable** | ⚠️ SUCCESS: Map 'X_C' (Race C - 100 HP) flashed to VIN 123456 |
| **Fixed** | ✅ ERROR: Invalid VIN format. Flash operation aborted. |

#### Test 4b: Decimal Number
**Steps:**
1. Connect to ROAD ECU (VIN 123456)
2. Select race map X_B
3. Enter: `123.5`

**Expected Results:**

| Version | Result |
|---------|--------|
| **Vulnerable** | ⚠️ SUCCESS: Map 'X_B' (Race B - 77 HP) flashed |
| **Fixed** | ✅ ERROR: Invalid VIN format. Flash operation aborted. |

#### Test 4c: Empty Input
**Steps:**
1. Connect to ROAD ECU
2. Select race map X_A
3. Just press Enter/OK without typing

**Expected Results:**

| Version | Result |
|---------|--------|
| **Vulnerable** | ⚠️ SUCCESS: Map 'X_A' (Race A - 65 HP) flashed |
| **Fixed** | ✅ ERROR: Invalid VIN format. Flash operation aborted. |

#### Test 4d: Too Short Number
**Steps:**
1. Connect to ROAD ECU
2. Select race map X_C
3. Enter: `123`

**Expected Results:**

| Version | Result |
|---------|--------|
| **Vulnerable** | ⚠️ SUCCESS: Race map flashed |
| **Fixed** | ✅ ERROR: VIN must be a 6-digit number (100000-999999) |

---

### Test 5: Verification Feature

**Steps:**
1. Connect to ROAD ECU
2. Use vulnerable version
3. Flash race map X_C using malformed input `ABC`
4. Click "Verify Flash Process"

**Expected Result:**
- ❌ VIOLATION DETECTED: ROAD ECU has RACE map (X_C)!
- Dialog with option to ignore

---

## Code Comparison

### Vulnerable Version (Default - `#else` block)

```c
scanf("%d", &vin_verification);  // ❌ Return value ignored!

int current_vin_num = atoi(current_vin);

if (vin_verification == current_vin_num) {
    if (current_ecu_type == ECU_ROAD && map->is_race_map) {
        printf("ERROR: Race maps not allowed...\n");
        return;
    }
} else if (vin_verification >= 100000 && vin_verification <= 999999) {
    printf("ERROR: VIN mismatch...\n");
    return;
}
// ⚠️ Falls through if vin_verification is -1
```

### Fixed Version (`#ifdef FIXED` block)

```c
// Reset to invalid value
vin_verification = -1;

// Read and CHECK return value
int scan_result = scanf("%d", &vin_verification);

// Validate scanf succeeded
if (scan_result != 1) {
    printf("ERROR: Invalid VIN format...\n");
    // Clear input buffer
    while ((c = getchar()) != '\n' && c != EOF);
    return;
}

// Validate range
if (vin_verification < 100000 || vin_verification > 999999) {
    printf("ERROR: VIN must be 6-digit...\n");
    return;
}

// Now check VIN match
if (vin_verification == current_vin_num) {
    if (current_ecu_type == ECU_ROAD && map->is_race_map) {
        printf("ERROR: Race maps not allowed...\n");
        return;
    }
} else {
    printf("ERROR: VIN mismatch...\n");
    return;
}
```

---

## Key Differences

| Aspect | Vulnerable | Fixed |
|--------|------------|-------|
| scanf return check | ❌ No | ✅ Yes |
| Range validation | ❌ Implicit (in else-if) | ✅ Explicit |
| Input buffer clear | ❌ No | ✅ Yes |
| Malformed input | ⚠️ Bypasses checks | ✅ Rejected |
| CERT C ERR33-C | ❌ Violates | ✅ Complies |
| CERT C EXP33-C | ❌ Violates | ✅ Complies |

---

## Automated Test Script

You can create a test script to verify both versions:

### test_vulnerability.bat
```batch
@echo off
echo ========================================
echo Testing ECU Simulator Vulnerability
echo ========================================
echo.

echo Building both versions...
call build_both.bat

echo.
echo Test 1: Vulnerable Version
echo Input: ABC
echo Expected: BYPASS (race map flashes)
echo.
echo Run this manually with run.bat

echo.
echo Test 2: Fixed Version  
echo Input: ABC
echo Expected: ERROR (rejected)
echo.
echo Run this manually with run_fixed.bat

pause
```

---

## For Parasoft/Static Analysis

### Scan Vulnerable Version
```batch
cd src
mingw32-make clean
mingw32-make
# Run Parasoft scan on ecu_sim.exe
```

Expected findings:
- ERR33-C: scanf return value ignored
- EXP33-C: Using potentially uninitialized variable

### Scan Fixed Version
```batch
cd src
mingw32-make clean
mingw32-make fixed
# Run Parasoft scan on ecu_sim_fixed.exe
```

Expected result:
- ✅ No ERR33-C violations
- ✅ No EXP33-C violations
- ✅ Proper input validation

---

## Summary

The conditional compilation allows you to:
1. **Demonstrate the vulnerability** (default build)
2. **Show the fix** (build with `-DFIXED`)
3. **Compare side-by-side** (build both)
4. **Test extensively** without changing code
5. **Use in training** to show before/after

Just change `#ifdef FIXED` to `#ifdef VULNERABLE` if you want to flip the default behavior!
