#pragma once

#include "ast/query/query.h"          // query
#include "ast/query/query_provider.h" // query_provider
#include "compiler/parser.h"          // parser_ctxt
#include "ds/cbuffer.h"               // cbuffer
#include "intf/types.h"               // u32

////////////////////// SCANNER (chars -> tokens)

typedef struct
{
  enum
  {
    SS_START,   // Start here and finish here
    SS_IDENT,   // Parsing an identifier or magic string
    SS_STRING,  // Parsing a "string"
    SS_NUMBER,  // Parsing the integer part of an int or float
    SS_DECIMAL, // Parsing the right half of a dec
  } state;
} compiler_state;

typedef struct
{
  bool ok;
  union
  {
    query query;
    error error; // A little big
  };
} compiler_result;

typedef struct
{
  compiler_state state;

  // Input and outputs
  cbuffer input;
  cbuffer output;

  char _input[10];
  compiler_result _output[10];

  /////////// SCANNER
  // Internal room to grow strings for tokens
  char str[512];
  u32 slen;

  /////////// PARSER
  // AST Stack for an LL1 parser
  // ast_parser parser_stack[20];
  // u32 sp;

  // Allocator for temporary variables in parser
  lalloc parser_work;
  u8 _parser_work[2048];
  parser_ctxt ctxt;

  query_provider *qp;
} compiler;

void compiler_create (compiler *dest, query_provider *qp);

void compiler_execute_all (compiler *s);
