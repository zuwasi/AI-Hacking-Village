# Conditional Compilation: Vulnerable vs Fixed

## Overview

The ECU simulator source code (`src/ecu_sim.c`) now contains **BOTH** the vulnerable and fixed versions using C preprocessor conditional compilation with `#ifdef FIXED`.

---

## How It Works

### Single Source File Structure

```c
// src/ecu_sim.c

void flash_map(char* map_id) {
    // ... setup code ...
    
    printf("PROMPT: Re-enter VIN...\n");
    
#ifdef FIXED
    /* ========================================
     * FIXED VERSION - Proper Input Validation
     * ======================================== */
    
    // Reset to invalid value
    vin_verification = -1;
    
    // Check scanf return value ✅
    int scan_result = scanf("%d", &vin_verification);
    if (scan_result != 1) {
        printf("ERROR: Invalid VIN format...\n");
        // Clear buffer
        while ((c = getchar()) != '\n' && c != EOF);
        return;
    }
    
    // Validate range ✅
    if (vin_verification < 100000 || vin_verification > 999999) {
        printf("ERROR: VIN must be 6-digit...\n");
        return;
    }
    
    // Check VIN match
    if (vin_verification == current_vin_num) {
        if (current_ecu_type == ECU_ROAD && map->is_race_map) {
            printf("ERROR: Race maps not allowed...\n");
            return;
        }
    } else {
        printf("ERROR: VIN mismatch...\n");
        return;
    }
    
#else
    /* ========================================
     * VULNERABLE VERSION - Intentional Bug
     * ======================================== */
    
    scanf("%d", &vin_verification);  // ❌ Return value ignored!
    
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
    
#endif
    
    printf("SUCCESS: Map flashed...\n");
}
```

---

## Compilation Commands

### Manual GCC Commands

**Vulnerable version (default):**
```bash
gcc -Wall -Wextra -std=c99 -c ecu_sim.c -o ecu_sim.o
gcc -o ecu_sim.exe ecu_sim.o
```

**Fixed version (with -DFIXED):**
```bash
gcc -Wall -Wextra -std=c99 -DFIXED -c ecu_sim.c -o ecu_sim_fixed.o
gcc -o ecu_sim_fixed.exe ecu_sim_fixed.o
```

### Using Makefile

**Build vulnerable:**
```bash
mingw32-make
# or
mingw32-make all
```

**Build fixed:**
```bash
mingw32-make fixed
```

**Build both:**
```bash
mingw32-make both
```

**Clean:**
```bash
mingw32-make clean
```

### Using Batch Files (Recommended)

| Command | Result |
|---------|--------|
| `build.bat` | Builds `ecu_sim.exe` (vulnerable) |
| `build_fixed.bat` | Builds `ecu_sim_fixed.exe` (fixed) |
| `build_both.bat` | Builds both executables |

---

## How Preprocessor Works

### Without -DFIXED (Default)

The preprocessor sees:
```c
#ifdef FIXED  // FALSE - FIXED is not defined
    // This code is SKIPPED
#else
    // This code is COMPILED ✅
    scanf("%d", &vin_verification);  // Vulnerable version
    // ...
#endif
```

**Result:** Vulnerable version compiled into `ecu_sim.exe`

### With -DFIXED

The preprocessor sees:
```c
#ifdef FIXED  // TRUE - FIXED is defined by -DFIXED flag
    // This code is COMPILED ✅
    int scan_result = scanf("%d", &vin_verification);
    if (scan_result != 1) { /* ... */ }
    // ...
#else
    // This code is SKIPPED
#endif
```

**Result:** Fixed version compiled into `ecu_sim_fixed.exe`

---

## Verification

### Check Which Version Is Running

**Vulnerable version behavior:**
```
Input: ABC
Output: SUCCESS: Map flashed  ← ⚠️ BYPASSED!
```

**Fixed version behavior:**
```
Input: ABC
Output: ERROR: Invalid VIN format. Flash operation aborted.  ← ✅ BLOCKED!
```

### View Compiled Code

**Using objdump (if available):**
```bash
objdump -d ecu_sim.exe > vulnerable_disasm.txt
objdump -d ecu_sim_fixed.exe > fixed_disasm.txt
diff vulnerable_disasm.txt fixed_disasm.txt
```

**Using strings:**
```bash
strings ecu_sim.exe | grep "Invalid VIN format"
# (empty - string not in vulnerable version)

strings ecu_sim_fixed.exe | grep "Invalid VIN format"  
# ERROR: Invalid VIN format. Flash operation aborted.
```

---

## Benefits of This Approach

### 1. **Single Source of Truth**
- One `ecu_sim.c` file contains both versions
- No duplicate code maintenance
- Easy to keep synchronized

### 2. **Clear Documentation**
- Comments in code explain each version
- Side-by-side comparison
- Training-friendly

### 3. **Easy Testing**
- Build both versions from same source
- Compare behavior directly
- Verify fix works

### 4. **Static Analysis**
- Scan vulnerable version → should find ERR33-C
- Scan fixed version → should be clean
- Demonstrates value of tools

### 5. **Training Material**
- Show "before" and "after"
- Demonstrate fix impact
- Prove vulnerability exists

---

## Common Preprocessor Patterns

### Alternative: Use Different Macro Names

```c
#ifdef VULNERABLE
    // Vulnerable code
#elif defined(FIXED)
    // Fixed code
#else
    // Default version
#endif
```

### Alternative: Multiple Fixes

```c
#if defined(FIX_SCANF)
    // Fix scanf only
#elif defined(FIX_COMPLETE)
    // Complete fix
#else
    // Vulnerable
#endif
```

### Alternative: Debug Modes

```c
#ifdef DEBUG
    printf("DEBUG: vin_verification = %d\n", vin_verification);
#endif
```

---

## Makefile Targets Explained

### Default Target
```makefile
all: $(TARGET)
```
Builds `ecu_sim.exe` without any defines → vulnerable version

### Fixed Target
```makefile
fixed: $(TARGET_FIXED)

$(OBJ_FIXED): $(SRC)
    $(CC) $(CFLAGS) -DFIXED -c $(SRC) -o $(OBJ_FIXED)
```
Adds `-DFIXED` flag → builds fixed version

### Both Target
```makefile
both: $(TARGET) $(TARGET_FIXED)
```
Builds both executables in sequence

---

## Best Practices

### 1. **Clear Comments**
✅ DO:
```c
#ifdef FIXED
    /* FIXED VERSION - Proper validation */
#else
    /* VULNERABLE VERSION - Demo only */
#endif
```

❌ DON'T:
```c
#ifdef FIXED
    // better code
#else
    // old code
#endif
```

### 2. **Consistent Naming**
✅ Use: `ecu_sim.exe` and `ecu_sim_fixed.exe`
❌ Don't use: `ecu_sim_v1.exe`, `ecu_sim_new.exe`

### 3. **Document Differences**
- Explain what each version does
- Why the vulnerability exists
- How the fix resolves it

### 4. **Test Both Versions**
- Ensure vulnerable version actually has the bug
- Ensure fixed version actually blocks it
- Compare side-by-side

---

## Integration with CI/CD

```yaml
# .github/workflows/build.yml
jobs:
  build-vulnerable:
    runs-on: windows-latest
    steps:
      - run: mingw32-make
      
  build-fixed:
    runs-on: windows-latest
    steps:
      - run: mingw32-make fixed
      
  test-vulnerability:
    needs: [build-vulnerable, build-fixed]
    steps:
      - run: test_both_versions.bat
```

---

## Summary

The conditional compilation approach provides:

✅ **Maintainability** - One source file  
✅ **Clarity** - Clear separation of versions  
✅ **Flexibility** - Easy to build either version  
✅ **Training Value** - Side-by-side comparison  
✅ **Testing** - Verify fix effectiveness  

Use `#ifdef FIXED` to control which version gets compiled!
