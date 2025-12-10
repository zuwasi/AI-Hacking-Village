# Quick Vulnerability Test

## Setup
```batch
build_both.bat
```

## Test the Vulnerability

### Test VULNERABLE Version

**Run:**
```batch
run.bat
```

**Steps:**
1. Click "Connect" until you get ROAD ECU (123456 or 234567)
2. Select race map: **X_C (Race C, 100HP)**
3. Red warning appears: "⚠️ WARNING: You are trying to flash an ILLEGAL RACE MAP to a ROAD ECU!"
4. Click "Flash Selected Map"
5. When prompted for VIN, type: **`ABC`** ← malformed input
6. Click OK

**Result:**
```
⚠️ WARNING: Flashing race map (100HP) on ROAD ECU may violate regulations!
SUCCESS: Map 'X_C' (Race C - 100 HP, 100HP) flashed to VIN 123456
```

✅ **VULNERABILITY CONFIRMED!** Race map bypassed security checks.

7. Click "Verify Flash Process"
8. See: **❌ VIOLATION DETECTED: ROAD ECU has RACE map (X_C)!**

---

### Test FIXED Version

**Run:**
```batch
run_fixed.bat
```

**Steps:**
1. Click "Connect" until you get ROAD ECU
2. Select race map: **X_C**
3. Click "Flash Selected Map"
4. When prompted, type: **`ABC`**
5. Click OK

**Result:**
```
ERROR: Invalid VIN format. Flash operation aborted.
```

✅ **FIX CONFIRMED!** Malformed input rejected.

**Try valid but wrong VIN:**
1. Same setup
2. Enter: **`234567`** (valid format, wrong VIN)

**Result:**
```
ERROR: VIN mismatch. Please try again.
```

✅ Valid VINs still work correctly.

**Try correct VIN:**
1. Same setup  
2. Enter: **`123456`** (correct VIN)

**Result:**
```
ERROR: Race maps not allowed on ROAD ECUs (regulatory violation)
```

✅ Race map protection still works!

---

## Side-by-Side Comparison

| Input | Vulnerable Version | Fixed Version |
|-------|-------------------|---------------|
| `ABC` (letters) | ⚠️ **BYPASS - SUCCESS** | ✅ ERROR: Invalid format |
| `123.5` (decimal) | ⚠️ **BYPASS - SUCCESS** | ✅ ERROR: Invalid format |
| Empty (just Enter) | ⚠️ **BYPASS - SUCCESS** | ✅ ERROR: Invalid format |
| `123` (too short) | ⚠️ **BYPASS - SUCCESS** | ✅ ERROR: Must be 6-digit |
| `234567` (wrong VIN) | ✅ ERROR: VIN mismatch | ✅ ERROR: VIN mismatch |
| `123456` (correct VIN + race map) | ✅ ERROR: Race maps not allowed | ✅ ERROR: Race maps not allowed |

---

## The Fix Explained

**Vulnerable Code:**
```c
scanf("%d", &vin_verification);  // ❌ Ignores return value
// If scanf fails, vin_verification stays at -1
// Falls through both checks → SUCCESS!
```

**Fixed Code:**
```c
int scan_result = scanf("%d", &vin_verification);
if (scan_result != 1) {  // ✅ Checks if scanf succeeded
    printf("ERROR: Invalid VIN format...\n");
    return;  // ✅ Rejects malformed input
}
if (vin_verification < 100000 || vin_verification > 999999) {
    printf("ERROR: VIN must be 6-digit...\n");
    return;  // ✅ Validates range
}
```

---

## Quick Command Reference

| Command | Purpose |
|---------|---------|
| `build.bat` | Build vulnerable version only |
| `build_fixed.bat` | Build fixed version only |
| `build_both.bat` | Build both versions |
| `run.bat` | Run with vulnerable version |
| `run_fixed.bat` | Run with fixed version |
| `kill_ecu.bat` | Kill any stuck ecu_sim.exe processes |

---

**Total test time: ~2 minutes**
