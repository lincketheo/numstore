#pragma once

#include "compiler/tokens.h"
#include "core/ds/cbuffer.h"               // err_t
#include "numstore/query/query_provider.h" // query_provider

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

  union
  {
    // SS_IDENT, SS_STRING, SS_INTEGER, SS_DECIMAL
    struct
    {
      u32 slen;
      char str[512];
    };
  };
} scanner_state;

typedef struct
{
  scanner_state state; // The state for resumability
  cbuffer *input;      // Chars
  cbuffer *output;     // tokens
  char prev_token;     // Char cache for 2 char lookahead for small tokens
  error e;             // Scanner related errors
  query_provider *qp;  // Allocates queries when an op code is encountered
  lalloc *dest;        // Where to allocate dynamic content when forwarding tokens (e.g. strings)
} scanner;

void scanner_init (
    scanner *dest,
    cbuffer *input,
    cbuffer *output,
    query_provider *qp,
    lalloc *dest_alloc);
void scanner_execute (scanner *s);
