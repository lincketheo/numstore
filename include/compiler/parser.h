#pragma once

#include "compiler/stack_parser/stack_parser.h"
#include "ds/cbuffer.h"
#include "services/services.h"

typedef struct
{
  cbuffer *tokens_input;
  cbuffer *queries_output;

  stack_parser sp;
} parser;

typedef struct
{
  lalloc *type_allocator;
  lalloc *stack_allocator;

  cbuffer *tokens_input;
  cbuffer *queries_output;
} parser_params;

/**
 * Returns:
 *   - ERR_NOMEM if you don't have enough room to allocate the AST stack
 */
err_t parser_create (parser *dest, parser_params params);

void parser_execute (parser *p);
