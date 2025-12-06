# CERT-C AI Demo - Code Analysis and Security Fixes

**Date:** December 6, 2025  
**Project:** C:\CERTC-AI-demo\src  
**Analyzer:** Amp AI Code Analysis

---

## 1. Project Overview

### Description
Motorcycle ECU Programming Simulator - A tool designed to flash ignition maps to motorcycle Engine Control Units (ECUs) with intentional security vulnerabilities for demonstration purposes.

### Main Components
- **ecu_sim.c** - ECU programming tool simulator
- **Makefile** - Build configuration
- **reports/report.xml** - Parasoft static analysis results

### Commands Available
- `IDENTIFY` - Connect to random motorcycle
- `GET_ALLOWED_MAPS` - List allowed ignition maps for current VIN
- `FLASH_MAP <map_id>` - Flash ignition map with VIN verification
- `QUIT` - Exit simulator

### Map Types
- **Road Maps** (R_A, R_B, R_C): 54HP maximum (regulatory compliant)
- **Race Maps** (X_A, X_B, X_C): 65-100HP (race ECUs only)

---

## 2. Initial Vulnerability Analysis

### Primary Vulnerability: Unchecked scanf() Return Value

**Location:** Line 180 in `flash_map()` function  
**File:** [ecu_sim.c](file:///C:/CERTC-AI-demo/src/ecu_sim.c#L180)

#### Original Code:
```c
printf("PROMPT: Re-enter VIN from motorcycle documents for verification:\n");
fflush(stdout);

scanf("%d", &vin_verification);
```

#### Issue:
The `scanf()` return value is ignored. When users enter malformed input (e.g., "123.5", "-123", "ABC", empty string), scanf fails but leaves `vin_verification` at its previous value (999999). This allows bypassing VIN verification entirely.

#### Impact:
- Allows flashing race maps (>54HP) onto ROAD ECUs
- Violates regulatory safety constraints
- Critical security vulnerability

#### Parasoft Violations Detected:
- **CERT_C-ERR33-a** (Severity 1): "The value returned by the standard library function 'scanf' should be used"
- **CERT_C-EXP12-a** (Severity 3): "Unused function's 'scanf' return value"
- **CERT_C-POS54-a** (Severity 1): "The value returned by the POSIX library function 'scanf' should be used"
- **CERT_C-ERR02-a** (Severity 3): "The 'scanf' library function should not be used"
- **CERT_C-STR07-a** (Severity 1): "Unsafe string function 'scanf' is being used"
- **CERT_C-INT05-a** (Severity 3): "Unsafe string function 'scanf' is being used"

---

## 3. Fix Applied: Conditional Compilation for QA Testing

### Implementation Strategy
Applied conditional compilation to allow QA testing of both vulnerable and fixed versions.

### Modified Code (Lines 177-193):
```c
printf("PROMPT: Re-enter VIN from motorcycle documents for verification:\n");
fflush(stdout);

#ifdef FIX
int scan_result = scanf("%d", &vin_verification);
if (scan_result != 1) {
    /* Clear input buffer on failed scan */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    printf("ERROR: Invalid VIN format. Please enter numeric VIN only.\n");
    fflush(stdout);
    return;
}
#else
scanf("%d", &vin_verification);
#endif
```

### Makefile Update:
```makefile
CC ?= gcc
CFLAGS = -Wall -Wextra -std=c99
# Uncomment to enable fix: CFLAGS += -DFIX
LD ?= $(CC)
```

### Usage:
- **Default build**: `make` - Original vulnerable behavior
- **Fixed build**: Uncomment `CFLAGS += -DFIX` in Makefile, then `make`
- **Command line**: `make CFLAGS="-Wall -Wextra -std=c99 -DFIX"`

### Fix Behavior:
When FIX is enabled:
1. Checks scanf return value
2. Validates exactly 1 integer was read
3. Clears input buffer on failure
4. Returns error message for invalid input
5. Prevents VIN verification bypass

---

## 4. Highest Priority Remaining Issues

### Severity 3 (Critical) Violations:

#### 4.1 Buffer Overflow Risk - strcpy() (Line 106)
**Violations:**
- CERT_C-API01-b: "Avoid using unsafe string function 'strcpy'"
- CERT_C-MSC24-c/d: "Unsafe functions 'strcpy' is being used"
- CERT_C-EXP12-a: "Unused function's 'strcpy' return value"

**Code:**
```c
strcpy(current_vin, vehicle->vin);
```

**Risk:** Potential buffer overflow when copying VIN

#### 4.2 Global Variables Not in Headers (Lines 69-75)
**Violations:**
- CERT_C-DCL15-a (multiple instances)

**Variables:**
- `current_vin` (line 69)
- `current_ecu_type` (line 70)
- `current_min_license` (line 71)
- `vin_verification` (line 73)
- `log_buffer` (line 75)
- `maps` (line 54)
- `vehicles` (line 63)

#### 4.3 Thread-Unsafe Function (Line 78)
**Violation:** CERT_C-CON33-a  
**Code:**
```c
srand(time(NULL));
```
**Issue:** srand() should not be used in multithreading applications

#### 4.4 Signed Modulo Operation (Line 103)
**Violation:** CERT_C-INT10-a  
**Code:**
```c
int selected_idx = rand() % MAX_VINS;
```
**Issue:** Left-hand operand of '%' should be unsigned

---

## 5. CRITICAL DISCOVERY: Backdoor Vulnerability

### 5.1 Backdoor Description

**Location:** Lines 197-203 in `flash_map()` function  
**Type:** "Else-less" validation bypass backdoor

### 5.2 Vulnerable Code Logic:

```c
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
// MISSING ELSE CLAUSE - BACKDOOR!
```

### 5.3 Validation Flow Analysis:

1. **IF** `vin_verification == current_vin_num` → Check race map restrictions ✓
2. **ELSE IF** `vin_verification` is in range [100000, 999999] → Reject as mismatch ✓
3. **ELSE** (MISSING) → **No validation, execution continues** ❌

### 5.4 Backdoor Exploitation:

**Exploit Method:** Enter any VIN number **outside** the range 100000-999999

**Examples of bypass values:**
- `0`
- `99999`
- `1000000`
- `-1`
- `42`

**Result:** Complete bypass of ALL security checks, allowing:
- Race maps on road ECUs
- Regulatory violations
- Unrestricted map flashing

### 5.5 Why the scanf() Fix Doesn't Close the Backdoor:

The `#ifdef FIX` only validates that scanf successfully reads an integer. It does NOT validate the VIN range logic. The backdoor remains exploitable even with the fix enabled because:
- Fix ensures valid integer input
- Fix does NOT enforce VIN range validation
- Missing `else` clause still allows out-of-range bypass

---

## 6. Static Analysis Detection of Backdoor

### 6.1 Parasoft Detection

**Rule:** CERT_C-MSC01-a  
**Severity:** 2 (Medium)  
**Line:** 190  
**Message:** "Provide 'else' after the last 'else-if' construct"

**XML Report Entry:**
```xml
<StdViol msg="Provide 'else' after the last 'else-if' construct" 
         ln="190" sev="2" auth="danie" 
         rule="CERT_C-MSC01-a" tool="c++test" 
         cat="CERT_C-MSC01" lang="cpp" locType="sr" 
         config="1" hash="1145304834" 
         locStartln="190" locStartPos="11" 
         locEndLn="190" locEndPos="12" 
         locFile="/ecu_simulator/ecu_sim.c"/>
```

### 6.2 Additional Supporting Violations (Line 190):

- **CERT_C-DCL06-a** (Sev 3): "Do not use the literal constant '100000'"
- **CERT_C-DCL06-a** (Sev 3): "Do not use the literal constant '999999'"
- **CERT_C-EXP00-a** (Sev 3): Parenthesis needed for clarity in compound conditions

### 6.3 Detection Significance:

The **CERT_C-MSC01-a** violation is a **direct indicator** of the backdoor vulnerability. Static analysis successfully identified the missing else clause that creates the security bypass, though it was classified as medium severity rather than critical.

---

## 7. Summary of Findings

### Vulnerabilities Identified:
1. ✅ **FIXED**: Unchecked scanf() return value (Line 180) - Conditional fix applied
2. ❌ **CRITICAL**: Backdoor via missing else clause (Line 190-203) - **NOT FIXED**
3. ❌ **HIGH**: Unsafe strcpy() buffer overflow risk (Line 106)
4. ❌ **MEDIUM**: Multiple global variables without headers
5. ❌ **MEDIUM**: Thread-unsafe srand() usage

### Security Status:
- **scanf vulnerability**: Mitigated with conditional compilation
- **Backdoor**: Remains exploitable in both FIX and NO-FIX modes
- **Buffer overflow**: Unaddressed
- **Code quality**: Multiple CERT-C violations remain

### Static Analysis Effectiveness:
- Successfully detected scanf issue (multiple rules)
- **Successfully detected backdoor** via CERT_C-MSC01-a
- Identified 100+ CERT-C violations total
- Mix of severity levels (1-3)

---

## 8. Recommendations

### Immediate Actions Required:

1. **Fix Backdoor Vulnerability:**
   ```c
   if (vin_verification == current_vin_num) {
       if (current_ecu_type == ECU_ROAD && map->is_race_map) {
           printf("ERROR: Race maps not allowed on ROAD ECUs\n");
           return;
       }
   } else if (vin_verification >= 100000 && vin_verification <= 999999) {
       printf("ERROR: VIN mismatch. Please try again.\n");
       return;
   } else {
       // NEW: Explicit rejection of out-of-range VINs
       printf("ERROR: Invalid VIN range. VIN must be 100000-999999.\n");
       return;
   }
   ```

2. **Replace strcpy() with strncpy():**
   ```c
   strncpy(current_vin, vehicle->vin, MAX_VIN_LEN - 1);
   current_vin[MAX_VIN_LEN - 1] = '\0';
   ```

3. **Make globals static or declare in header:**
   ```c
   static char current_vin[MAX_VIN_LEN] = "";
   static ECUType current_ecu_type;
   static int current_min_license = 0;
   static int vin_verification = 999999;
   ```

4. **Replace magic numbers with named constants:**
   ```c
   #define MIN_VIN 100000
   #define MAX_VIN 999999
   #define LOG_BUFFER_SIZE 512
   ```

### Testing Recommendations:

1. Test scanf fix with malformed inputs (text, decimals, empty)
2. Test backdoor with out-of-range VINs (0, 99999, 1000000)
3. Verify race map restrictions on road ECUs
4. Fuzz test all user input paths
5. Re-run static analysis after fixes

---

## 9. Files Modified

### C:\CERTC-AI-demo\src\ecu_sim.c
- Added conditional compilation for scanf fix (#ifdef FIX)
- Lines 181-193 modified

### C:\CERTC-AI-demo\src\Makefile
- Added comment with instructions to enable FIX flag
- Line 3 modified

---

## 10. Conclusion

This analysis identified two critical security vulnerabilities in the ECU simulator:

1. **Input validation bypass** via unchecked scanf() - now conditionally fixed
2. **Backdoor** via missing else clause - **remains unpatched and exploitable**

The Parasoft static analysis tool successfully detected both vulnerabilities, demonstrating the value of automated security scanning. The backdoor detection via **CERT_C-MSC01-a** rule is particularly significant, as it identified a deliberate security flaw through coding standard enforcement.

**Key Takeaway:** Even when one vulnerability is fixed (scanf), other vulnerabilities (backdoor) can remain. Comprehensive security requires addressing all findings, not just the most obvious ones. The missing `else` clause is a subtle but critical flaw that completely undermines the VIN verification security mechanism.

---

**Analysis conducted by:** Amp AI  
**Report generated:** December 6, 2025  
**Repository:** https://github.com/zuwasi/My_CUDA_Profiler (working directory context)
