#pragma once

#include "dev/assert.h"
#include "intf/mm.h"
#include "sds.h"
#include "typing.h"

////////////////////// SCANNER (chars -> tokens)

typedef enum
{
  SS_START,
  SS_CHAR_COLLECT,
  SS_NUMBER_COLLECT,
  SS_DECIMAL_COLLECT,
  SS_ERROR_REWIND,
} scanner_state;

typedef struct
{
  cbuffer *chars_input;
  cbuffer *tokens_output;
  u32 current;
  scanner_state state;

  char *dcur;            // Current data for variable length data
  u32 dcurlen;           // len of dcur
  u32 dcurcap;           // capacity of dcur
  lalloc *strings_alloc; // Allocator for strings
} scanner;

DEFINE_DBG_ASSERT_H (scanner, scanner, s);

// TODO - use database to request resources
void scanner_create (
    scanner *dest,
    cbuffer *input,
    cbuffer *output,
    lalloc *strings_alloc);
void scanner_execute (scanner *s); // Buffer will be empty at the end

////////////////////// PARSER

typedef enum
{
  PS_START,
  PS_CREATE,
  PS_ERROR_REWIND,
} parser_state;

typedef struct
{
  cbuffer *tokens_input;  // The input buffer for tokens
  cbuffer *opcode_output; // The output for op codes
  cbuffer *stack_output;  // The stack output for the vm

  lalloc *input_strings; // The string allocator
  lalloc *tokens;        // The type allocator

  type *currenttype;  // While building types
  parser_state state; // Current state for execute
} parser;

DEFINE_DBG_ASSERT_H (parser, parser, p);
// TODO use database to request resources
void parser_create (
    parser *dest,
    cbuffer *input,
    cbuffer *output,
    lalloc *input_strings,
    lalloc *tokens);
void parser_execute (parser *s);
