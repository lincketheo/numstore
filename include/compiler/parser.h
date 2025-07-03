#pragma once

#include "compiler/ast/statement.h" // statement
#include "compiler/tokens.h"
#include "core/ds/cbuffer.h"
#include "core/mm/lalloc.h"

typedef struct
{
  void *yyp;         // The lemon parser
  lalloc *work;      // Where to allocate work
  u32 tnum;          // Token number in the current statement
  error *e;          // Return status
  statement *result; // Resulting statement
  bool done;         // Done parsing this query
} parser_result;

err_t parser_result_create (parser_result *ret, lalloc *work, error *e);
statement *parser_result_consume (parser_result *p);
err_t parser_result_parse (parser_result *res, token tok, error *e);

typedef struct
{
  union
  {
    statement *stmt;
    error e;
  };
  bool ok;
} statement_result;

typedef struct
{
  cbuffer *input;       // tokens input
  cbuffer *output;      // statement_result output
  parser_result parser; // One for each statement
  error e;              // parser related errors passed forward
  lalloc parser_work;   // Work for parser - to be copied to query space
  u8 _parser_work[2048];
  bool in_err;

#ifndef NDEBUG
  bool expect_opcode;
#endif
} parser;

void parser_init (parser *dest, cbuffer *input, cbuffer *output);
void parser_execute (parser *p);
