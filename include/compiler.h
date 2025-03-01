#pragma once

#include "dev/assert.h"
#include "sds.h"

/**
 * Compiler Architecture
 * Data In (chars) -> Scanner -> Tokens -> Parser -> ByteCode -> VM
 */

typedef enum
{
  // Token starts with alpha
  TT_WRITE,
  TT_IDENTIFIER,

  // Token starts with num
  TT_INTEGER,
  TT_FLOAT,

  // Types
  TT_U32,

  // Single chars
  TT_SEMICOLON,
} token_t;

#define TT_IS_SINGLE(t) (t == TT_SEMICOLON)

typedef struct
{
  union
  {
    string str;
    i32 integer;
    f32 floating;
  };
  u8 type;
} token;

////////////////////// SCANNER (chars -> tokens)
typedef struct
{
  cbuffer *chars_input;
  cbuffer *tokens_output;

  u32 current;
  int is_error;
} scanner;

DEFINE_DBG_ASSERT_H (scanner, scanner, s);
void scanner_create (scanner *dest, cbuffer *input, cbuffer *output);
void scanner_execute (scanner *s); // Scans as many tokens as it can

////////////////////// TOKEN PRINTER
void token_log_info (token t);
typedef struct
{
  cbuffer *tokens_input;
} token_printer;

void token_printer_create (token_printer *dest, cbuffer *input);
void token_printer_execute (token_printer *t);

////////////////////// PARSER
typedef struct
{
  cbuffer *tokens_input;
  cbuffer *bytecode_output;
  int is_error;
} parser;

DEFINE_DBG_ASSERT_H (parser, parser, p);
void parser_create (parser *dest, cbuffer *input, cbuffer *output);
void parser_execute (parser *s);
