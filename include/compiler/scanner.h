#pragma once

#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "intf/mm.h"

////////////////////// SCANNER (chars -> tokens)

typedef enum
{
  SS_START,
  SS_IDENT,
  SS_STRING,
  SS_NUMBER,
  SS_DECIMAL,
  SS_ERROR_REWIND,
} scanner_state;

const char *scanner_state_to_str (scanner_state);

typedef struct
{
  // Input and output buffers
  cbuffer *chars_input;
  cbuffer *tokens_output;

  // Current state
  scanner_state state;

  // Internal growing string
  char *dcur;  // Current data for variable length data
  u32 dcurlen; // len of dcur
  u32 dcurcap; // capacity of dcur

  lalloc *string_allocator;
} scanner;

typedef struct
{
  cbuffer *chars_input;
  cbuffer *tokens_output;
  lalloc *string_allocator;
} scanner_params;

void scanner_create (scanner *dest, scanner_params params);

void scanner_execute (scanner *s);

void scanner_release (scanner *dest);
