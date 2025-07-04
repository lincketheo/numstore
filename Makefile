###############################
##### Default Target
all: debug                     # `make` = debug build

###############################
##### Compiler & Global Flags
CC      ?= gcc
CFLAGS   = -I./include

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
PARSER_C     := ./src/compiler/parser.c      

$(LEMON): $(LEMON_SRC)
	$(CC) -o $@ $<

$(PARSER_C): $(LEMON) $(GRAMMAR) $(LEMPAR)
	@echo "  LEMON   $(@F)"
	@$(LEMON) $(GRAMMAR) -T$(LEMPAR) -d./tools/lemon || true
	@cp ./tools/lemon/parser.c $@

###############################
##### Source Discovery
SRC       := $(shell find src -type f -name '*.c') $(PARSER_C)
OBJ       := $(SRC:.c=.o)

###############################
##### Apps 
APPS      := nstorec nstores test cursor nspprint analyze

APP_SRC   := $(addprefix apps/,$(addsuffix .c,$(APPS)))
APP_OBJ   := $(APP_SRC:.c=.o)

###############################
##### Build Modes
debug:   CFLAGS += $(shell cat debug_flags.txt 2>/dev/null)
release: CFLAGS += $(shell cat release_flags.txt 2>/dev/null || echo "-O2 -DNDEBUG")

debug  : $(APPS)
release: clean $(APPS)

###############################
##### Compile Rule
%.o: %.c | $(PARSER_C)
	$(CC) $(CFLAGS) -c $< -o $@ $(LFLAGS)

###############################
##### Link Targets
$(APPS): %: apps/%.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

###############################
##### Utilities
clean:
	rm -f $(APPS) $(OBJ) $(APP_OBJ) \
	      $(PARSER_C) $(LEMON) 

format:
	clang-format -i $(shell find src include apps -name '*.c' -o -name '*.h')

.PHONY: all debug release clean format

