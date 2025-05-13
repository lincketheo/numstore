#pragma once

#include "compiler/stack_parser/ast_parser.h" // ast_parser
#include "stmtctrl.h"                         // stmtctrl

#include "ds/cbuffer.h" // cbuffer
#include "mm/lalloc.h"  // lalloc

typedef struct
{
  cbuffer *tokens_input;
  cbuffer *queries_output;

  ast_parser stack[20];
  u32 sp;

  u8 _working_space[2048];
  lalloc working_space;

  stmtctrl *ctrl;
} parser;

void parser_create (
    parser *dest,
    cbuffer *tokens_input,
    cbuffer *queries_output,
    stmtctrl *ctrl);

void parser_execute (parser *p);
