#pragma once

#include "types.hpp"

typedef enum
{
  T_WRITE,
  T_IDENT,
  T_INTEGER,
  T_LEFT_BRACKET,
  T_RIGHT_BRACKET,
  T_LEFT_PAREN,
  T_RIGHT_PAREN,
  T_COMMA,
  T_EOF,
} token_t;

typedef struct
{
  union
  {
    int int_val;
  };
  const char *literal;
  usize llen;
  int pos;
  token_t type;
} token;

typedef struct
{
  token *tokens;
  usize len;
  usize cap;
} token_arr;

#define token_arr_assert(t)                                                   \
  assert (t);                                                                 \
  assert ((t)->len <= (t)->cap);                                              \
  assert ((t)->tokens);                                                       \
  assert ((t)->cap > 0);

void token_arr_print (token_arr *t);

result<token_arr> token_arr_create ();

void token_arr_free (token_arr *arr);

token token_create (const char *loc, usize len, token_t type, int pos);

result<void> token_arr_push (token_arr *t, token tok);
