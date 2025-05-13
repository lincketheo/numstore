#pragma once

#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "mm/lalloc.h"

////////////////////// SCANNER (chars -> tokens)

typedef enum
{
  SS_START,
  SS_IDENT,
  SS_STRING,
  SS_NUMBER,
  SS_DECIMAL,
} scanner_state;

const char *scanner_state_to_str (scanner_state);

typedef struct
{
  error e;
  scanner_state state;

  /**
   * Shared input output buffers
   */
  cbuffer *chars_input;   // A buffer of chars
  cbuffer *tokens_output; // A buffer of token_msg

  /**
   * Internal growing string (free'd downstream)
   */
  char *dcur;  // Current data for variable length data
  u32 dcurlen; // len of dcur
  u32 dcurcap; // capacity of dcur

  /**
   * Shared allocator for growing string
   */
  lalloc *string_allocator;
} scanner;

typedef struct
{
  cbuffer *chars_input;
  cbuffer *tokens_output;
  lalloc *string_allocator;
} scanner_params;

scanner scanner_create (scanner_params params);
void scanner_execute (scanner *s);
