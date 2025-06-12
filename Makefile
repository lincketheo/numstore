UNAME_S := $(shell uname -s)

CC := gcc
BASE_CFLAGS := -I./include -Wall -Wextra -Werror -pedantic

ifeq ($(UNAME_S), Darwin)
	BASE_CFLAGS += -D_DARWIN_C_SOURCE
endif

# Source files
SRC := $(shell find ./src -type f -name "*.c")
OBJ := $(SRC:.c=.o)

SRC_APPS := $(wildcard apps/*.c)
APP_BIN := $(patsubst apps/%.c,%,$(SRC_APPS))

# Format and Lint
FORMAT_FILES := $(shell find . -type f \( -name "*.c" -o -name "*.h" \))

# Default: debug build
all: ./src/compiler/parser.c debug 

# Debug build target
debug: CFLAGS := $(BASE_CFLAGS) -O0 -g
debug: $(APP_BIN)

# Release build target
release: CFLAGS := $(BASE_CFLAGS) -O3 -DNDEBUG
release: $(APP_BIN)

# Rule to build each app binary
%: $(OBJ) apps/%.o
	$(CC) $(CFLAGS) -o $@ $^

# Generic object compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

./src/compiler/parser.c: ./tools/lemon/lemon ./tools/lemon/parser.y
	./tools/lemon/lemon ./tools/lemon/parser.y -d./tools/lemon -T./tools/lemon/lempar.c
	mv ./tools/lemon/parser.c ./src/compiler/parser.c 
	rm ./tools/lemon/parser.h

./tools/lemon/lemon: ./tools/lemon/lemon.c 
	gcc -o $@ $^

# Utility targets
clean:
	rm -rf $(OBJ)
	rm -f $(addprefix apps/,$(APP_BIN:=.o))
	rm -f $(APP_BIN)
	rm -f *.db
	rm -f ./src/compiler/parser.c
	rm -f ./tools/lemon/lemon
	rm -f ./tools/lemon/parser.c
	rm -f ./tools/lemon/parser.h
	rm -f ./tools/lemon/parser.out

host-docs:
	cd docs && npm run dev

format:
	clang-format -i $(FORMAT_FILES)

lint:
	clang-tidy --warnings-as-errors=* --quiet \
	  $(shell find ./src -name '*.c') -- \
	  $(BASE_CFLAGS)

.PHONY: all clean format lint debug release

