CC            := gcc
CFLAGS.base   := -I./include -Wall -Wextra -pedantic
ifeq ($(shell uname -s),Darwin)
CFLAGS.base  += -D_DARWIN_C_SOURCE
endif

LEMON        := ./tools/lemon/lemon
LEMON_SRC    := ./tools/lemon/lemon.c
GRAMMAR      := ./tools/lemon/parser.y
LEMPAR       := ./tools/lemon/lempar.c
PARSER_C     := ./src/compiler/parser.c

SRC          := $(shell find src -type f -name '*.c') $(PARSER_C)
OBJ          := $(SRC:.c=.o)

APP_SRC      := $(wildcard apps/*.c)
APP_OBJ      := $(APP_SRC:.c=.o)
APP_BIN      := $(patsubst apps/%.c,%,$(APP_SRC))

all: debug

debug: CFLAGS := $(CFLAGS.base) -O0 -g
debug: $(APP_BIN)

release: CFLAGS := $(CFLAGS.base) -O3 -DNDEBUG
release: $(APP_BIN)

$(LEMON): $(LEMON_SRC)
	@echo "  CC      $@"
	@$(CC) -o $@ $<

$(PARSER_C): $(LEMON) $(GRAMMAR) $(LEMPAR)
	@echo "  LEMON   $(notdir $@)"
	@$(LEMON) $(GRAMMAR) -T$(LEMPAR) -d./tools/lemon || true
	@cp tools/lemon/parser.c $@

%.o: %.c
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -c $< -o $@

apps/%.o: apps/%.c
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%: $(OBJ) apps/%.o
	@echo "  LD      $@"
	@$(CC) $(CFLAGS) -o $@ $^

clean:
	@echo "  CLEAN"
	rm -f $(OBJ) $(APP_OBJ) $(APP_BIN) $(PARSER_C) $(LEMON) 
	rm -f tools/lemon/parser.c
	rm -f tools/lemon/parser.h
	rm -f tools/lemon/parser.out

format:
	clang-format -i $(shell find src include -name '*.c' -o -name '*.h')

lint:
	clang-tidy --warnings-as-errors=* --quiet $(filter-out $(PARSER_C),$(SRC)) -- $(CFLAGS.base)

.PHONY: all debug release clean format lint

