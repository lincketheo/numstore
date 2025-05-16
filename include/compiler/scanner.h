#pragma once

#include "ast/query/qspace_provider.h"
#include "ds/cbuffer.h" // cbuffer
#include "intf/types.h" // u32

////////////////////// SCANNER (chars -> tokens)

typedef enum
{
  SS_START,   // Start here and finish here
  SS_IDENT,   // Parsing an identifier or magic string
  SS_STRING,  // Parsing a "string"
  SS_NUMBER,  // Parsing the integer part of an int or float
  SS_DECIMAL, // Parsing the right half of a dec
} scanner_state;

typedef struct
{
  scanner_state state;

  cbuffer *chars_input;
  cbuffer *tokens_output;

  /**
   * Internal room to grow strings
   */
  char str[512];
  u32 slen;

  qspce_prvdr *qspcp; // To allocate query spaces on op codes
  query *cur;         // Current query to allocate data onto
} scanner;

scanner scanner_create (
    cbuffer *chars_input,
    cbuffer *tokens_output,
    qspce_prvdr *qspcp);

err_t scanner_execute (scanner *s, error *e);
