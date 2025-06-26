#pragma once

#include <stddef.h> // size_t

#include "core/ds/cbuffer.h"
#include "core/errors/error.h" // TODO
#include "core/intf/io.h"      // TODO
#include "core/mm/lalloc.h"    // TODO

#include "compiler/tokens.h" // TODO

#include "numstore/query/query_provider.h" // TODO
#include "numstore/type/types.h"           // TODO

typedef struct
{
  void *yyp;    // The lemon parser
  lalloc *work; // Where to allocate work
  lalloc *dest; // Allocate the final stuff
  u32 tnum;     // Token number
  error *e;     // Return status
  query result; // Result query
  bool ready;   // Done parsing this query
} parser_result;

// Create a parser context
err_t parser_create (parser_result *ret, lalloc *work, lalloc *dest, error *e);
query parser_consume (parser_result *p);
err_t parser_parse (parser_result *res, token tok, error *e);

typedef struct
{
  cbuffer *input;       // tokens
  cbuffer *output;      // queries
  parser_result parser; // allocated on each op code
  error e;              // parser related errors passed forward
  lalloc parser_work;   // Work for parser - to be copied to query space
  u8 _parser_work[2048];
  bool in_err;
} parser;

void parser_init (
    parser *dest,
    cbuffer *input,
    cbuffer *output);
void parser_execute (parser *p);
