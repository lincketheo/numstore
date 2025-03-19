#include "compiler/tokens.hpp"

#include <cstdlib>

#define INITIAL_CAP 10

#define token_assert(t) assert (t)

#define __tttostr_case(t)                                                     \
case t:                                                                     \
  return #t;

#define ttforeach(func)                                                       \
func (T_WRITE);                                                             \
func (T_IDENT);                                                             \
func (T_INTEGER);                                                           \
func (T_LEFT_BRACKET);                                                      \
func (T_RIGHT_BRACKET);                                                     \
func (T_LEFT_PAREN);                                                        \
func (T_RIGHT_PAREN);                                                       \
func (T_COMMA);                                                             \
func (T_COLON);                                                             \
func (T_EOF);

static const char *
tttostr (token_t t)
{
  switch (t)
  {
    ttforeach (__tttostr_case);
  }
  return NULL;
}

token
token_create (const char *loc, usize len, token_t type, int pos)
{
  token ret = {
    .literal = loc,
    .llen = len,
    .pos = pos,
    .type = type,
  };

  if (type == T_INTEGER)
  {
    // Implicit assert(ret.literal is integer) because we already scanned it
    ret.int_val = atoi (ret.literal);
  }

  return ret;
}

static void
token_print (token *t)
{
  token_assert (t);
  printf ("%s %.*s\n", tttostr (t->type), (int)t->llen, t->literal);
}

void
token_arr_print (token_arr *arr)
{
  token_arr_assert (arr);

  for (int i = 0; i < arr->len; ++i)
  {
    token t = arr->tokens[i];
    token_print (&t);
  }
}

result<token_arr>
token_arr_create ()
{
  token_arr ret;
  ret.tokens = (token *)malloc (INITIAL_CAP * sizeof *ret.tokens);
  if (ret.tokens == nullptr)
  {
    perror ("malloc");
    return err<token_arr> ();
  }
  ret.len = 0;
  ret.cap = INITIAL_CAP;
  return ok (ret);
}

void
token_arr_free (token_arr *arr)
{
  token_arr_assert (arr);
  free (arr->tokens);
  arr->len = 0;
  arr->cap = 0;
}

result<void>
token_arr_push (token_arr *t, token tok)
{
  token_arr_assert (t);

  // TODO - this only needs 1 realloc
  while (t->len + 1 >= t->cap)
  {
    token *next = (token *)realloc (t->tokens, t->cap * 2 * sizeof*t->tokens);
    if (next == nullptr)
    {
      return err<void> ();
    }
    t->tokens = next;
    t->cap = t->cap * 2;
  }

  t->tokens[t->len++] = tok;

  return ok_void ();
}
