###############################
##### Default Target
all: debug                     # `make` = debug build

###############################
##### Compiler & Global Flags
CC      ?= gcc
CFLAGS   = -I./include         # single include path for every file

# Build objects as PIC when SHARED=1 (default)
SHARED ?= 1
ifeq ($(SHARED),1)
  CFLAGS += -fPIC
endif

###############################
##### Lemon Code Generation
LEMON        := ./tools/lemon/lemon
LEMON_SRC    := ./tools/lemon/lemon.c
GRAMMAR      := ./tools/lemon/parser.y
LEMPAR       := ./tools/lemon/lempar.c
PARSER_C     := ./src/compiler/parser.c      # generated parser

$(LEMON): $(LEMON_SRC)
	@echo "  CC      $@"
	@$(CC) -o $@ $<

$(PARSER_C): $(LEMON) $(GRAMMAR) $(LEMPAR)
	@echo "  LEMON   $(@F)"
	@$(LEMON) $(GRAMMAR) -T$(LEMPAR) -d./tools/lemon || true
	@cp ./tools/lemon/parser.c $@

###############################
##### Source Discovery
SRC       := $(shell find src -type f -name '*.c') $(PARSER_C)
OBJ       := $(SRC:.c=.o)

###############################
##### Apps (explicit list — no wildcard)
APPS      := nstorec nstores test

APP_SRC   := $(addprefix apps/,$(addsuffix .c,$(APPS)))
APP_OBJ   := $(APP_SRC:.c=.o)

###############################
##### Build Modes
debug:   CFLAGS += $(shell cat debug_flags.txt 2>/dev/null)
release: CFLAGS += $(shell cat release_flags.txt 2>/dev/null || echo "-O2 -DNDEBUG")

debug  : $(APPS)
release: $(APPS)

###############################
##### Compile Rule
# Order-only prerequisite ensures parser is generated first
%.o: %.c | $(PARSER_C)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -c $< -o $@

###############################
##### Link Targets
# Each app links its own object + every object from src/
$(APPS): %: apps/%.o $(OBJ)
	@echo "  LD      $@"
	@$(CC) $(CFLAGS) -o $@ $^

###############################
##### Utilities
clean:
	rm -f $(APPS) $(OBJ) $(APP_OBJ) \
	      $(PARSER_C) $(LEMON) 

format:
	clang-format -i $(shell find src include apps -name '*.c' -o -name '*.h')

.PHONY: all debug release clean format

