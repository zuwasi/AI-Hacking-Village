CC ?= gcc
CFLAGS = -Wall -Wextra -std=c99
LD ?= $(CC)
LDFLAGS =
TARGET = ecu_sim.exe
SRC = ecu_sim.c
OBJ = ecu_sim.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJ)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

clean:
	del /Q $(TARGET) $(OBJ) 2>nul

.PHONY: all clean
