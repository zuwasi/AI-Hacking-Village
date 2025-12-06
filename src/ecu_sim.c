/*
 * ECU Simulator - Motorcycle Engine Control Unit Programming Tool
 * 
 * INTENTIONAL BUG LOCATION:
 * --------------------------
 * Function: flash_map() around line 175-195
 * Issue: scanf() return value is ignored when reading VIN verification
 * Impact: When user enters malformed input (e.g., "123.5", "ABC", empty),
 *         scanf fails but leaves vin_verification at its previous value (-1).
 *         The check skips both VIN verification and race map protection because
 *         -1 is outside the valid VIN range (100000-999999).
 *         This allows flashing race maps (>54HP) onto ROAD ECUs, violating
 *         regulatory constraints.
 * 
 * Additional CERT C violations (non-critical noise):
 * - strcpy usage instead of strncpy
 * - Missing const on read-only parameters
 * - Ignoring printf return values
 * - Global variables that could be local
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

void init_system() {
    srand(time(NULL));
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

void cmd_identify() {
    int selected_idx = rand() % MAX_VINS;
    VehicleInfo* vehicle = &vehicles[selected_idx];
    
    strcpy(current_vin, vehicle->vin);
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

void cmd_get_allowed_maps() {
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
    
    VehicleInfo* vehicle = NULL;
    int i;
    for (i = 0; i < MAX_VINS; i++) {
        if (strcmp(vehicles[i].vin, current_vin) == 0) {
            vehicle = &vehicles[i];
            break;
        }
    }
    
    printf("PROMPT: Re-enter VIN from motorcycle documents for verification:\n");
    fflush(stdout);
    
    scanf("%d", &vin_verification);
    
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
    
    int num_tokens = sscanf(command, "%s %s", cmd, arg);
    
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
