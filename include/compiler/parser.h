#pragma once

#include "compiler/stack_parser/stack_parser.h" // stack_parser
#include "ds/cbuffer.h"                         // cbuffer
#include "mm/lalloc.h"                          // lalloc

typedef struct
{
  cbuffer *tokens_input;
  cbuffer *queries_output;
  stack_parser sp;
  bool is_error;
} parser;

typedef struct
{
  lalloc *type_allocator;
  cbuffer *tokens_input;
  cbuffer *queries_output;
} parser_params;

parser parser_create (parser_params params);

void parser_execute (parser *p);
