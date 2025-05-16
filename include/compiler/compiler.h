#pragma once

#include "ast/query/qspace_provider.h" // qspce_prvdr
#include "compiler/parser.h"           // parser
#include "compiler/scanner.h"          // scanner
#include "compiler/tokens.h"           // token

#include "ast/query/query.h" // query
#include "ds/cbuffer.h"      // cbuffer
#include "errors/error.h"    // err_t

typedef struct
{
  enum
  {
    COMPR_ERROR,
    COMPR_QUERY,
  } type;
  union
  {
    error e;
    query *q;
  };
} compiler_result;

typedef struct
{
  cbuffer *chars_input;
  cbuffer tokens;
  cbuffer *query_output;

  scanner scanner;
  parser parser;

  token _tokens[10];
} compiler;

void compiler_create (
    compiler *dest,
    cbuffer *chars_input,
    cbuffer *queries_output,
    qspce_prvdr *qspcp);

bool compiler_done (compiler *c);

err_t compiler_execute (compiler *c, error *e);
