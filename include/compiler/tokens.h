#pragma once

#include "core/ds/strings.h" // TODO

#include "numstore/query/queries/create.h" // TODO
#include "numstore/query/query.h"          // TODO
#include "numstore/type/types.h"           // TODO

typedef enum
{
  //      Arithmetic Operators
  TT_PLUS = 1, // +
  TT_MINUS,    // -
  TT_SLASH,    // /
  TT_STAR,     // *

  //      Logical Operators
  TT_BANG,          // !
  TT_BANG_EQUAL,    // !=
  TT_EQUAL_EQUAL,   // ==
  TT_GREATER,       // >
  TT_GREATER_EQUAL, // >=
  TT_LESS,          // <
  TT_LESS_EQUAL,    // <=

  //       Fancy Operators
  TT_NOT,       // ~
  TT_CARET,     // ^
  TT_PERCENT,   // %
  TT_PIPE,      // |
  TT_AMPERSAND, // &

  //        Other One char tokens
  TT_SEMICOLON,     // ;
  TT_COLON,         // :
  TT_LEFT_BRACKET,  // [
  TT_RIGHT_BRACKET, // ]
  TT_LEFT_BRACE,    // {
  TT_RIGHT_BRACE,   // }
  TT_LEFT_PAREN,    // (
  TT_RIGHT_PAREN,   // )
  TT_COMMA,         // ,

  //      Other
  TT_STRING,     // "[A-Za-z0-9]*"$
  TT_IDENTIFIER, // ^[A-Za-z][A-Za-z0-9]*$

  // Tokens that start with a number or +/-
  TT_INTEGER, // integer
  TT_FLOAT,   // float

  //      Identifiers first, then literals identified later
  //      Literal Operations
  TT_CREATE, // create
  TT_DELETE, // delete
  TT_INSERT, // insert

  //      Type literals
  TT_STRUCT, // struct
  TT_UNION,  // union
  TT_ENUM,   // enum
  TT_PRIM,   // prim

  //      Bools
  TT_TRUE,  // true
  TT_FALSE, // false

  //      Special Error token on failed scan
  TT_ERROR,
} token_t;

#define case_OPCODE \
  TT_CREATE:        \
  case TT_DELETE:   \
  case TT_INSERT

static inline bool
tt_is_opcode (token_t type)
{
  switch (type)
    {
    case case_OPCODE:
      {
        return true;
      }
    default:
      {
        return false;
      }
    }
}

query_t tt_to_qt (token_t t);

/**
 * A token is a tagged union that wraps
 * the value and the type together
 */
typedef struct
{
  union
  {
    string str;   // STRING or IDENTIFIER
    i32 integer;  // TT_INTEGER
    f32 floating; // TT_FLOAT
    prim_t prim;  // TT_PRIM
    query q;      // TT_OP_CODE
    error e;      // TT_ERROR
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

#define tt_opcode(op, _q) \
  (token) { .type = op, .q = _q }

#define tt_err(_e) \
  (token) { .type = TT_ERROR, .e = _e }

#define MAX_TOK_T_LEN 16

bool token_equal (const token *left, const token *right);

const char *tt_tostr (token_t t);
