#pragma once

#include "compiler/ast/statement.h" // statement
#include "core/ds/cbuffer.h"        // cbuffer

typedef struct
{
  enum
  {
    SS_START,   // Start here and finish here
    SS_IDENT,   // Parsing an identifier or magic string
    SS_STRING,  // Parsing a "string"
    SS_NUMBER,  // Parsing the integer part of an int or float
    SS_DECIMAL, // Parsing the right half of a dec
    SS_ERR,     // Rewinding to next TT_SEMICOLON
  } state;

  // Used in SS_IDENT, SS_STRING, SS_INTEGER, SS_DECIMAL
  u32 slen;
  char str[512];
} scanner_state;

typedef struct
{
  scanner_state state; // The state for resumability
  cbuffer *input;      // Input Chars
  cbuffer *output;     // Output tokens
  char prev_token;     // Char cache for 2 char lookahead for small tokens
  error e;             // Scanner related errors
  statement *current;  // Current Statement
} scanner;

void scanner_init (scanner *dest, cbuffer *input, cbuffer *output);
void scanner_execute (scanner *s);
