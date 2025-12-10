CC ?= gcc
CFLAGS = -Wall -Wextra -std=c99
LD ?= $(CC)
LDFLAGS =
TARGET = ecu_sim.exe
TARGET_FIXED = ecu_sim_fixed.exe
SRC = ecu_sim.c
OBJ = ecu_sim.o
OBJ_FIXED = ecu_sim_fixed.o

# Default target: build vulnerable version
all: $(TARGET)

# Build vulnerable version (default)
$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJ)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Build fixed version with FIXED defined
fixed: $(TARGET_FIXED)

$(TARGET_FIXED): $(OBJ_FIXED)
	$(LD) $(LDFLAGS) -o $(TARGET_FIXED) $(OBJ_FIXED)

$(OBJ_FIXED): $(SRC)
	$(CC) $(CFLAGS) -DFIXED -c $(SRC) -o $(OBJ_FIXED)

# Build both versions
both: $(TARGET) $(TARGET_FIXED)

clean:
	del /Q $(TARGET) $(TARGET_FIXED) $(OBJ) $(OBJ_FIXED) 2>nul

.PHONY: all fixed both clean
