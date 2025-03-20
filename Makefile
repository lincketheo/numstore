FORMAT_FILES := $(shell find . -type f \( -name "*.cpp" -o -name "*.hpp" \))

SRC := $(shell find ./src -type f -name "*.cpp")
OBJ := $(SRC:.cpp=.o)

SRC_APPS := $(shell find ./apps -type f -name "*.cpp")
OBJ_APPS := $(SRC_APPS:.cpp=.o)

CXX_FLAGS := -std=c++20 -I./include -g

all: client server test tokenizer

client: $(OBJ) apps/client.cpp
	g++ $(CXX_FLAGS) -o $@ $^ 

server: $(OBJ) apps/server.cpp
	g++ $(CXX_FLAGS) -o $@ $^ 

tokenizer: $(OBJ) apps/tokenizer.cpp
	g++ $(CXX_FLAGS) -o $@ $^ 

test: $(OBJ) apps/test.o 
	g++ $(CXX_FLAGS) -o $@ $^ 

%.o: %.cpp
	g++ $(CXX_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ)
	rm -rf $(OBJ_APPS)
	rm -f client 
	rm -f server
	rm -f test 
	rm -f tokenizer

format:
	clang-format -i $(FORMAT_FILES)

lint:
	find . -name "*.cpp" -or -name "*.hpp" | xargs clang-tidy --warnings-as-errors=* --quiet

.PHONY: format
