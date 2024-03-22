# Makefile for building the "writer" application.

## Compiler and compiler flags
CC := $(if $(CROSS_COMPILE), $(CROSS_COMPILE)gcc, gcc)
CFLAGS := -Wall -Wextra -std=c99

## Source files
SRC_FILES := writer.c
OBJ_FILES := $(patsubst %.c, %.o, $(SRC_FILES))

## Target executable
TARGET := writer

# Build the writer application
all: $(TARGET) cross-compile-result

# Build rule for native platform
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

# Build rule for cross-compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target: remove generated files
clean:
	rm -rf $(OBJ_FILES) $(TARGET) ../fileresult.txt

# Command for creating fileresult.txt
cross-compile-result:
ifneq ($(CROSS_COMPILE),)
	file writer >> ../fileresult.txt
endif

.PHONY: all clean cross-compile-result
