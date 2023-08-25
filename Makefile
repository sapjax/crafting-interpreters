# Set the compiler
CC = gcc
# Set compilation flags
CFLAGS = -Wall -Wextra

# Define source and build directory paths
SRC_DIR = src
BUILD_DIR = build

# Get the list of source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Generate the list of object files based on source files
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Specify the name of the generated executable
TARGET = my_program

# Default target, compile the executable
all: $(BUILD_DIR) $(TARGET)

# Compile source files into object files and generate the executable
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/$@

# Create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean the generated files
clean:
	rm -rf $(BUILD_DIR)

lint:
	clang-tidy $(SRCS) -- $(CFLAGS)

.PHONY: all clean
