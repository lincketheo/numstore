#pragma once

#include "ast/query/query.h"          // query
#include "ast/query/query_provider.h" // query_provider
#include "compiler/parser.h"          // parser_ctxt
#include "ds/cbuffer.h"               // cbuffer
#include "intf/types.h"               // u32

/**
 * A single compiler combines both scan and parse steps
 */
typedef struct compiler_s compiler;

typedef struct
{
  bool ok;
  union
  {
    query query;
    error error;
  };
} compiler_result;

compiler *compiler_create (query_provider *qp, error *e);
cbuffer *compiler_get_input (compiler *c);
cbuffer *compiler_get_output (compiler *c);
void compiler_execute (compiler *s);
