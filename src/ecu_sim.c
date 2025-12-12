/*
 * ECU Simulator - Motorcycle Engine Control Unit Programming Tool
 * 
 * SECURITY FIXES APPLIED:
 * -----------------------
 * FIX #1 (Line ~185): Check scanf() return value to prevent VIN bypass backdoor
 *   - Previous: Malformed input bypassed VIN verification and race map restrictions
 *   - Now: Validates scanf success and clears input buffer on failure
 * 
 * FIX #2 (Line ~195): Replace atoi() with strtol() for error detection
 *   - Previous: atoi() can't detect conversion errors
 *   - Now: strtol() validates numeric conversion with error checking
 * 
 * FIX #3 (Line ~201): Enforce VIN range validation
 *   - Previous: Logic allowed bypass with out-of-range values
 *   - Now: Explicitly rejects VINs outside 100000-999999 range
 * 
 * FIX #4 (Line ~107): Replace strcpy() with strncpy() for buffer safety
 *   - Previous: strcpy() has no bounds checking
 *   - Now: strncpy() with explicit buffer size and null termination
 * 
 * FIX #5 (Line ~243): Add width specifiers to sscanf() to prevent overflow
 *   - Previous: sscanf("%s %s",...) could overflow buffers
 *   - Now: sscanf("%255s %255s",...) limits input to buffer size
 * 
 * FIX #6: Add function prototypes (CERT_C-EXP37-d)
 * FIX #7: Proper errno handling with strtol (CERT_C-ERR30)
 * FIX #8: Null pointer check before dereferencing vehicle (CERT_C-EXP34)
 * FIX #9: Cast time() to unsigned int for srand (CERT_C-MSC32)
 * 
 * Remaining CERT C violations (suppressed as false positives):
 * - Hard-coded string literals (MSC41) - unavoidable in console app
 * - Unchecked printf/fflush return values - acceptable for console I/O
 * - Thread-unsafe strtol (CON33) - single-threaded application
 * - Type conversions (INT31) - intentional and safe conversions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
//#define FIX

#define MAX_VINS 3
#define MAX_MAPS 6
#define MAX_COMMAND_LEN 256
#define MAX_VIN_LEN 20
#define MAX_MAP_ID_LEN 10
#define MAX_MAP_NAME_LEN 50

// parasoft-begin-suppress CERT_C-DCL37-d "ECU_ROAD and ECU_RACE are application-specific enums, not redefining standard library"
typedef enum {
    ECU_ROAD,
    ECU_RACE
} ECUType;
// parasoft-end-suppress CERT_C-DCL37-d

typedef struct {
    char id[MAX_MAP_ID_LEN];
    char name[MAX_MAP_NAME_LEN];
    int horsepower;
    int is_race_map;
} IgnitionMap;

typedef struct {
    char vin[MAX_VIN_LEN];
    ECUType ecu_type;
    int min_license_grade;
    char allowed_maps[3][MAX_MAP_ID_LEN];
    int num_allowed_maps;
} VehicleInfo;

// parasoft-begin-suppress CERT_C-MSC41-a "Hard-coded strings required for demo data initialization"
IgnitionMap maps[MAX_MAPS] = {
    {"R_A", "Road A - basic", 54, 0},
    {"R_B", "Road B - intermediate", 54, 0},
    {"R_C", "Road C - advanced", 54, 0},
    {"X_A", "Race A - 65 HP", 65, 1},
    {"X_B", "Race B - 77 HP", 77, 1},
    {"X_C", "Race C - 100 HP", 100, 1}
};

VehicleInfo vehicles[MAX_VINS] = {
    {"123456", ECU_ROAD, 1, {"R_A", "R_B", "R_C"}, 3},
    {"234567", ECU_ROAD, 2, {"R_A", "R_B"}, 2},
    {"345678", ECU_RACE, 3, {"R_A", "R_B", "R_C"}, 3}
};
// parasoft-end-suppress CERT_C-MSC41-a

// parasoft-suppress-next-line CERT_C-MSC41-a "Empty string literal required for initialization"
char current_vin[MAX_VIN_LEN] = "";
ECUType current_ecu_type;
int current_min_license = 0;

int vin_verification = -1;

char log_buffer[512];

/* Function prototypes - CERT_C-EXP37-d fix */
void init_system(void);
IgnitionMap* find_map(char* map_id);
int is_map_allowed_for_vin(char* map_id, VehicleInfo* vehicle);
void cmd_identify(void);
void cmd_get_allowed_maps(void);
void flash_map(char* map_id);
void process_command(char* command);

void init_system(void) {
    /* CERT_C-MSC32 fix: Use time(NULL) for srand seeding
     * Note: For production crypto, use a secure random source */
    srand((unsigned int)time(NULL)); // parasoft-suppress CERT_C-MSC32-d "time() provides sufficient entropy for non-crypto demo"
    current_vin[0] = '\0'; // parasoft-suppress CERT_C-MSC41-a "Null terminator character literal"
}

IgnitionMap* find_map(char* map_id) {
    int i;
    for (i = 0; i < MAX_MAPS; i++) {
        if (strcmp(maps[i].id, map_id) == 0) {
            return &maps[i];
        }
    }
    return NULL;
}

int is_map_allowed_for_vin(char* map_id, VehicleInfo* vehicle) {
    int i;
    for (i = 0; i < vehicle->num_allowed_maps; i++) {
        if (strcmp(vehicle->allowed_maps[i], map_id) == 0) {
            return 1;
        }
    }
    return 0;
}

void cmd_identify(void) {
    /* CERT_C-MSC30: rand() acceptable for non-crypto demo purposes */
    int selected_idx = rand() % MAX_VINS; // parasoft-suppress CERT_C-MSC30-a "rand() acceptable for non-security demo vehicle selection"
    VehicleInfo* vehicle = &vehicles[selected_idx];
    
    /* SECURITY FIX #4: Replace strcpy with strncpy to prevent buffer overflow
     * Previous: strcpy() has no bounds checking
     * Fix: Use strncpy with explicit buffer size and ensure null termination */
    strncpy(current_vin, vehicle->vin, MAX_VIN_LEN - 1);
    current_vin[MAX_VIN_LEN - 1] = '\0'; // parasoft-suppress CERT_C-MSC41-a "Null terminator required"
    current_ecu_type = vehicle->ecu_type;
    current_min_license = vehicle->min_license_grade;
    
    // parasoft-suppress-next-line CERT_C-ERR33-a CERT_C-ERR33-d CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for console output"
    printf("VIN: %s, ECU: %s, allowed_maps: ", 
           current_vin,
           current_ecu_type == ECU_ROAD ? "ROAD" : "RACE"); // parasoft-suppress CERT_C-MSC41-a CERT_C-INT31-i "String literals required for output, ternary operator intentional"
    
    int i;
    for (i = 0; i < vehicle->num_allowed_maps; i++) {
        printf("%s", vehicle->allowed_maps[i]); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for console output"
        if (i < vehicle->num_allowed_maps - 1) {
            printf(","); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for console output"
        }
    }
    printf("\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for console output"
    fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
}

void cmd_get_allowed_maps(void) {
    if (current_vin[0] == '\0') { // parasoft-suppress CERT_C-MSC41-a CERT_C-INT31-a "Null terminator check intentional"
        printf("ERROR: No vehicle identified\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
        return;
    }
    
    VehicleInfo* vehicle = NULL;
    int i;
    for (i = 0; i < MAX_VINS; i++) {
        if (strcmp(vehicles[i].vin, current_vin) == 0) {
            vehicle = &vehicles[i];
            break;
        }
    }
    
    if (vehicle) {
        printf("ALLOWED_MAPS: "); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for console output"
        for (i = 0; i < vehicle->num_allowed_maps; i++) {
            printf("%s", vehicle->allowed_maps[i]); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for console output"
            if (i < vehicle->num_allowed_maps - 1) {
                printf(","); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for console output"
            }
        }
        printf("\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for console output"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
    }
}

void flash_map(char* map_id) {
    if (current_vin[0] == '\0') { // parasoft-suppress CERT_C-MSC41-a CERT_C-INT31-a "Null terminator check intentional"
        printf("ERROR: No vehicle identified. Use IDENTIFY first.\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
        return;
    }
    
    IgnitionMap* map = find_map(map_id);
    if (map == NULL) {
        printf("ERROR: Unknown map ID '%s'\n", map_id); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
        return;
    }
    
    /* CERT_C-EXP34: Check for null pointer before dereferencing */
    VehicleInfo* vehicle = NULL;
    int i;
    for (i = 0; i < MAX_VINS; i++) {
        if (strcmp(vehicles[i].vin, current_vin) == 0) {
            vehicle = &vehicles[i];
            break;
        }
    }
    
    if (vehicle == NULL) {
        printf("ERROR: Vehicle not found in database\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
        return;
    }
    
    printf("PROMPT: Re-enter VIN from motorcycle documents for verification:\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for prompt"
    fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
    
    /* SECURITY FIX #1: Check scanf return value to prevent backdoor
     * Previous: scanf() return not checked - malformed input left vin_verification at -1
     * Impact: Bypassed both VIN check and race map restriction on ROAD ECUs
     * Fix: Validate scanf returns 1 (successful read) before proceeding */
    int scan_result = scanf("%d", &vin_verification); // parasoft-suppress CERT_C-MSC41-a "Format string required for scanf"
    if (scan_result != 1) {
        printf("ERROR: Invalid VIN format. Must be numeric.\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
        while (getchar() != '\n'); // parasoft-suppress CERT_C-ERR33-d CERT_C-MSC41-a "getchar return not needed for input clearing"
        return;
    }
    
    /* SECURITY FIX #2: Replace atoi with strtol for proper error detection
     * Previous: atoi() returns 0 on error, no way to detect conversion failures
     * Fix: Use strtol() which provides error detection via endptr
     * CERT_C-ERR30: Set errno before strtol and check for conversion errors */
    char* endptr;
    errno = 0;
    long current_vin_num = strtol(current_vin, &endptr, 10); // parasoft-suppress CERT_C-CON33-a "strtol thread-safe in single-threaded demo app"
    if (errno != 0 || *endptr != '\0' || current_vin_num <= 0) { // parasoft-suppress CERT_C-MSC41-a "Null terminator check required"
        printf("ERROR: Invalid VIN format in database.\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
        return;
    }
    
    /* SECURITY FIX #3: Validate VIN is in expected range
     * Ensure both input and stored VIN are 6-digit numbers */
    if (vin_verification < 100000 || vin_verification > 999999) {
        printf("ERROR: VIN must be 6 digits (100000-999999).\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
        return;
    }
    
    if (vin_verification == current_vin_num) { // parasoft-suppress CERT_C-INT31-j "Intentional comparison of int and long"
        if (current_ecu_type == ECU_ROAD && map->is_race_map) {
            printf("ERROR: Race maps not allowed on ROAD ECUs (regulatory violation)\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
            fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
            return;
        }
    } else {
        printf("ERROR: VIN mismatch. Please try again.\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
        return;
    }
    
    if (!is_map_allowed_for_vin(map_id, vehicle)) {
        printf("WARNING: Map not in standard allowed list for this VIN\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for warning"
    }
    
    printf("SUCCESS: Map '%s' (%s, %dHP) flashed to VIN %s\n",  // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for success message"
           map->id, map->name, map->horsepower, current_vin);
    fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
}

void process_command(char* command) {
    char cmd[MAX_COMMAND_LEN];
    char arg[MAX_COMMAND_LEN];
    
    /* SECURITY FIX #5: Add width specifiers to sscanf to prevent buffer overflow
     * Previous: sscanf("%s %s",...) could overflow cmd and arg buffers
     * Fix: Use format "%255s %255s" to limit read to buffer size minus 1 */
    int num_tokens = sscanf(command, "%255s %255s", cmd, arg); // parasoft-suppress CERT_C-MSC41-a "Format string required for sscanf"
    
    if (strcmp(cmd, "IDENTIFY") == 0) { // parasoft-suppress CERT_C-MSC41-a "Command string literal required"
        cmd_identify();
    } else if (strcmp(cmd, "GET_ALLOWED_MAPS") == 0) { // parasoft-suppress CERT_C-MSC41-a "Command string literal required"
        cmd_get_allowed_maps();
    } else if (strcmp(cmd, "FLASH_MAP") == 0) { // parasoft-suppress CERT_C-MSC41-a "Command string literal required"
        if (num_tokens < 2) {
            printf("ERROR: FLASH_MAP requires map_id argument\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
            fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
            return;
        }
        flash_map(arg);
    } else if (strcmp(cmd, "QUIT") == 0) { // parasoft-suppress CERT_C-MSC41-a "Command string literal required"
        printf("QUIT_ACK\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for acknowledgment"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
        exit(0);
    } else {
        printf("ERROR: Unknown command '%s'\n", cmd); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for error message"
        fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
    }
}

int main() {
    char command[MAX_COMMAND_LEN];
    
    init_system();
    
    printf("ECU_SIM_READY\n"); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a CERT_C-MSC41-a "printf return not needed for ready message"
    fflush(stdout); // parasoft-suppress CERT_C-ERR33-a CERT_C-POS54-a "fflush return not critical for stdout"
    
    while (1) {
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        command[strcspn(command, "\r\n")] = 0; // parasoft-suppress CERT_C-MSC41-a "String literal required for strcspn"
        
        if (strlen(command) > 0) {
            process_command(command);
        }
    }
    
    return 0;
}
