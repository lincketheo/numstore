#pragma once

#include "ast/query/query_provider.h"
#include "ast/type/types.h"
#include "compiler/tokens.h"
#include "errors/error.h"
#include "intf/io.h"
#include "mm/lalloc.h"

#include <stddef.h> // size_t

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
