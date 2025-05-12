#pragma once

#include "compiler/stack_parser/stack_parser.h" // stack_parser
#include "ds/cbuffer.h"                         // cbuffer
#include "mm/lalloc.h"                            // lalloc

typedef struct
{
  cbuffer *tokens_input;
  cbuffer *queries_output;
  stack_parser sp;
} parser;

typedef struct
{
  lalloc *alloc; // used for stack_parser
  cbuffer *tokens_input;
  cbuffer *queries_output;
} parser_params;

/**
 * Returns:
 *   - ERR_NOMEM if you don't have enough room to allocate the AST stack
 */
err_t parser_create (parser *dest, parser_params params, error *e);

void parser_execute (parser *p);
