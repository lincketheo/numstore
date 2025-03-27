# Detect OS (Linux or Darwin for macOS)
UNAME_S := $(shell uname -s)

# Compiler and Flags
CC := gcc
CFLAGS := -I./include -g -Wall -Wextra -Werror -pedantic -O0

ifeq ($(UNAME_S), Darwin)
	CFLAGS += -D_DARWIN_C_SOURCE
endif

# Source files
SRC := $(shell find ./src -type f -name "*.c")
OBJ := $(SRC:.c=.o)

SRC_APPS := $(wildcard apps/*.c)
APP_BIN := $(patsubst apps/%.c,%,$(SRC_APPS))

# Format and Lint
FORMAT_FILES := $(shell find . -type f \( -name "*.c" -o -name "*.hpp" \))

# Default target
all: $(APP_BIN)

# Rule to build each app binary
%: $(OBJ) apps/%.o
	$(CC) $(CFLAGS) -o $@ $^

# Generic object compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Utility targets
clean:
	rm -rf $(OBJ)
	rm -f $(addprefix apps/,$(APP_BIN:=.o))
	rm -f $(APP_BIN)

format:
	clang-format -i $(FORMAT_FILES)

lint:
	find . -name "*.c" -or -name "*.hpp" | xargs clang-tidy --warnings-as-errors=* --quiet

.PHONY: all clean format lint

