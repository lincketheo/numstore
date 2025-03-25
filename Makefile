FORMAT_FILES := $(shell find . -type f \( -name "*.c" -o -name "*.hpp" \))

SRC := $(shell find ./src -type f -name "*.c")
OBJ := $(SRC:.c=.o)

SRC_APPS := $(shell find ./apps -type f -name "*.c")
OBJ_APPS := $(SRC_APPS:.c=.o)

CXX_FLAGS := -I./include -g

all: test main 

test: $(OBJ) apps/test.o 
	gcc $(CXX_FLAGS) -o $@ $^ 

main: $(OBJ) apps/main.o 
	gcc $(CXX_FLAGS) -o $@ $^ 

%.o: %.c
	gcc $(CXX_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ)
	rm -rf $(OBJ_APPS)
	rm -f main
	rm -f test 

format:
	clang-format -i $(FORMAT_FILES)

lint:
	find . -name "*.c" -or -name "*.hpp" | xargs clang-tidy --warnings-as-errors=* --quiet

.PHONY: format
