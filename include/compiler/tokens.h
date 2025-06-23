#pragma once

#include "compiler/ast/query/queries/create.h"
#include "compiler/ast/query/query.h"
#include "compiler/ast/type/types.h"
#include "core/ds/strings.h"

typedef enum
{
  // Tokens that start with a letter (alpha)
  //      Json Operations
  TT_CREATE = 1,
  TT_DELETE,
  TT_APPEND,
  TT_INSERT,
  TT_UPDATE,
  TT_READ,
  TT_TAKE,

  //      Binary Operations
  TT_BCREATE,
  TT_BDELETE,
  TT_BAPPEND,
  TT_BINSERT,
  TT_BUPDATE,
  TT_BREAD,
  TT_BTAKE,

  //      Type Utils
  TT_STRUCT,
  TT_UNION,
  TT_ENUM,
  TT_PRIM,

  //      Other
  TT_IDENTIFIER,
  TT_STRING,

  // Tokens that start with a number or +/-
  TT_INTEGER,
  TT_FLOAT,

  // Tokens that are single characters
  TT_SEMICOLON,
  TT_LEFT_BRACKET,
  TT_RIGHT_BRACKET,
  TT_LEFT_BRACE,
  TT_RIGHT_BRACE,
  TT_LEFT_PAREN,
  TT_RIGHT_PAREN,
  TT_COMMA,
} token_t;

query_t tt_to_qt (token_t t);

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
    query q;
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

#define tt_string(val) \
  (token) { .type = TT_STRING, .str = val }

#define tt_prim(val) \
  (token) { .type = TT_PRIM, .prim = val }

#define MAX_TOK_T_LEN 16

const char *tt_tostr (token_t t);
