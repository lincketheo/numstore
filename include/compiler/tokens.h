#pragma once

#include "ds/strings.h"
#include "typing.h"

typedef enum
{
  // Tokens that start with a letter (alpha)
  TT_WRITE,
  TT_READ,
  TT_TAKE,
  TT_CREATE,
  TT_DELETE,
  TT_IDENTIFIER,

  // Tokens that start with a number or +/-
  TT_INTEGER,
  TT_FLOAT,

  // Types
  //      Complex
  TT_STRUCT,
  TT_UNION,
  TT_ENUM,
  TT_PRIM,

  // Tokens that are single characters
  TT_SEMICOLON,
  TT_LEFT_BRACKET,
  TT_RIGHT_BRACKET,
  TT_LEFT_BRACE,
  TT_RIGHT_BRACE,
  TT_LEFT_PAREN,
  TT_RIGHT_PAREN,
  TT_COMMA,

  // Special Tokens
  TT_ERROR, // An Error token, saying: next token start fresh
} token_t;

// Returns if this token is a single character
#define TT_IS_SINGLE(t) (t == TT_SEMICOLON)

/**
 * A token is a tagged union that wraps
 * the value and the type together
 */
typedef struct
{
  union
  {
    string str;
    i32 integer;
    f32 floating;
    prim_t prim;
  };
  token_t type;
} token;

// Shorthands
#define quick_tok(_type) \
  (token) { .type = _type }

#define tt_integer(val) \
  (token) { .type = TT_INTEGER, .integer = val }

#define tt_float(val) \
  (token) { .type = TT_FLOAT, .floating = val }

#define tt_ident(val) \
  (token) { .type = TT_IDENTIFIER, .str = val }

#define tt_prim(val) \
  (token) { .type = TT_PRIM, .prim = val }
