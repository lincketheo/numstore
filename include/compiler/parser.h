#pragma once

#include "ast/query/query.h"
#include "compiler/stack_parser/ast_parser.h" // ast_parser
#include "ds/cbuffer.h"                       // cbuffer
#include "mm/lalloc.h"                        // lalloc

typedef struct
{
  cbuffer *tokens_input;
  cbuffer *query_ptr_output;

  ast_parser stack[20];
  u32 sp;

  // Used for builders (linked lists etc) before they run build
  u8 _working_space[2048];
  lalloc working_space;

  query *cur; // Same as scanner - keep track of where to allocate things
} parser;

void parser_create (
    parser *dest,
    cbuffer *tokens_input,
    cbuffer *query_ptr_output);

err_t parser_execute (parser *p, error *e);
