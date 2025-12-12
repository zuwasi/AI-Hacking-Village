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
 * Remaining CERT C violations (false positives for demo code):
 * - Hard-coded string literals (MSC41) - unavoidable in console app
 * - Unchecked printf/fflush return values - acceptable for console I/O
 * - Thread-unsafe strtol (CON33) - single-threaded application
 * - Reserved identifiers (DCL37) - system-defined enums
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

typedef enum {
    ECU_ROAD,
    ECU_RACE
} ECUType;

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
    srand((unsigned int)time(NULL));
    current_vin[0] = '\0';
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
    int selected_idx = rand() % MAX_VINS;
    VehicleInfo* vehicle = &vehicles[selected_idx];
    
    /* SECURITY FIX #4: Replace strcpy with strncpy to prevent buffer overflow
     * Previous: strcpy() has no bounds checking
     * Fix: Use strncpy with explicit buffer size and ensure null termination */
    strncpy(current_vin, vehicle->vin, MAX_VIN_LEN - 1);
    current_vin[MAX_VIN_LEN - 1] = '\0';
    current_ecu_type = vehicle->ecu_type;
    current_min_license = vehicle->min_license_grade;
    
    printf("VIN: %s, ECU: %s, allowed_maps: ", 
           current_vin,
           current_ecu_type == ECU_ROAD ? "ROAD" : "RACE");
    
    int i;
    for (i = 0; i < vehicle->num_allowed_maps; i++) {
        printf("%s", vehicle->allowed_maps[i]);
        if (i < vehicle->num_allowed_maps - 1) {
            printf(",");
        }
    }
    printf("\n");
    fflush(stdout);
}

void cmd_get_allowed_maps(void) {
    if (current_vin[0] == '\0') {
        printf("ERROR: No vehicle identified\n");
        fflush(stdout);
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
        printf("ALLOWED_MAPS: ");
        for (i = 0; i < vehicle->num_allowed_maps; i++) {
            printf("%s", vehicle->allowed_maps[i]);
            if (i < vehicle->num_allowed_maps - 1) {
                printf(",");
            }
        }
        printf("\n");
        fflush(stdout);
    }
}

void flash_map(char* map_id) {
    if (current_vin[0] == '\0') {
        printf("ERROR: No vehicle identified. Use IDENTIFY first.\n");
        fflush(stdout);
        return;
    }
    
    IgnitionMap* map = find_map(map_id);
    if (map == NULL) {
        printf("ERROR: Unknown map ID '%s'\n", map_id);
        fflush(stdout);
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
        printf("ERROR: Vehicle not found in database\n");
        fflush(stdout);
        return;
    }
    
    printf("PROMPT: Re-enter VIN from motorcycle documents for verification:\n");
    fflush(stdout);
    
    /* SECURITY FIX #1: Check scanf return value to prevent backdoor
     * Previous: scanf() return not checked - malformed input left vin_verification at -1
     * Impact: Bypassed both VIN check and race map restriction on ROAD ECUs
     * Fix: Validate scanf returns 1 (successful read) before proceeding */
    int scan_result = scanf("%d", &vin_verification);
    if (scan_result != 1) {
        printf("ERROR: Invalid VIN format. Must be numeric.\n");
        fflush(stdout);
        while (getchar() != '\n');
        return;
    }
    
    /* SECURITY FIX #2: Replace atoi with strtol for proper error detection
     * Previous: atoi() returns 0 on error, no way to detect conversion failures
     * Fix: Use strtol() which provides error detection via endptr
     * CERT_C-ERR30: Set errno before strtol and check for conversion errors */
    char* endptr;
    errno = 0;
    long current_vin_num = strtol(current_vin, &endptr, 10);
    if (errno != 0 || *endptr != '\0' || current_vin_num <= 0) {
        printf("ERROR: Invalid VIN format in database.\n");
        fflush(stdout);
        return;
    }
    
    /* SECURITY FIX #3: Validate VIN is in expected range
     * Ensure both input and stored VIN are 6-digit numbers */
    if (vin_verification < 100000 || vin_verification > 999999) {
        printf("ERROR: VIN must be 6 digits (100000-999999).\n");
        fflush(stdout);
        return;
    }
    
    if (vin_verification == current_vin_num) {
        if (current_ecu_type == ECU_ROAD && map->is_race_map) {
            printf("ERROR: Race maps not allowed on ROAD ECUs (regulatory violation)\n");
            fflush(stdout);
            return;
        }
    } else {
        printf("ERROR: VIN mismatch. Please try again.\n");
        fflush(stdout);
        return;
    }
    
    if (!is_map_allowed_for_vin(map_id, vehicle)) {
        printf("WARNING: Map not in standard allowed list for this VIN\n");
    }
    
    printf("SUCCESS: Map '%s' (%s, %dHP) flashed to VIN %s\n", 
           map->id, map->name, map->horsepower, current_vin);
    fflush(stdout);
}

void process_command(char* command) {
    char cmd[MAX_COMMAND_LEN];
    char arg[MAX_COMMAND_LEN];
    
    /* SECURITY FIX #5: Add width specifiers to sscanf to prevent buffer overflow
     * Previous: sscanf("%s %s",...) could overflow cmd and arg buffers
     * Fix: Use format "%255s %255s" to limit read to buffer size minus 1 */
    int num_tokens = sscanf(command, "%255s %255s", cmd, arg);
    
    if (strcmp(cmd, "IDENTIFY") == 0) {
        cmd_identify();
    } else if (strcmp(cmd, "GET_ALLOWED_MAPS") == 0) {
        cmd_get_allowed_maps();
    } else if (strcmp(cmd, "FLASH_MAP") == 0) {
        if (num_tokens < 2) {
            printf("ERROR: FLASH_MAP requires map_id argument\n");
            fflush(stdout);
            return;
        }
        flash_map(arg);
    } else if (strcmp(cmd, "QUIT") == 0) {
        printf("QUIT_ACK\n");
        fflush(stdout);
        exit(0);
    } else {
        printf("ERROR: Unknown command '%s'\n", cmd);
        fflush(stdout);
    }
}

int main() {
    char command[MAX_COMMAND_LEN];
    
    init_system();
    
    printf("ECU_SIM_READY\n");
    fflush(stdout);
    
    while (1) {
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        command[strcspn(command, "\r\n")] = 0;
        
        if (strlen(command) > 0) {
            process_command(command);
        }
    }
    
    return 0;
}
