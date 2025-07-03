#include "compiler/compiler.h"
#include "compiler/parser.h"
#include "compiler/scanner.h"
#include "compiler/tokens.h"

struct compiler_s
{
  // outputs
  cbuffer chars;
  cbuffer tokens;
  cbuffer statements;

  char _chars[10];
  token _tokens[10];
  statement_result _statments[10];

  scanner s;
  parser p;
};

DEFINE_DBG_ASSERT_I (compiler, compiler, s)
{
  ASSERT (s);
}

static const char *TAG = "Compiler";

////////////////////////////// Main Functions

compiler *
compiler_create (error *e)
{
  // Allocate compiler
  compiler *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "%s Failed to allocate compiler", TAG);
      return NULL;
    }

  *ret = (compiler){
    .chars = cbuffer_create_from (ret->_chars),
    .tokens = cbuffer_create_from (ret->_tokens),
    .statements = cbuffer_create_from (ret->_statments),
  };

  scanner_init (&ret->s, &ret->chars, &ret->tokens);
  parser_init (&ret->p, &ret->tokens, &ret->statements);

  compiler_assert (ret);

  return ret;
}

void
compiler_free (compiler *c)
{
  compiler_assert (c);
  i_free (c);
}

cbuffer *
compiler_get_input (compiler *c)
{
  compiler_assert (c);
  return &c->chars;
}

cbuffer *
compiler_get_output (compiler *c)
{
  compiler_assert (c);
  return &c->statements;
}

void
compiler_execute (compiler *s)
{
  compiler_assert (s);

  while (true)
    {
      /**
       * Block on downstream
       */
      if (cbuffer_avail (&s->statements) < sizeof (query))
        {
          return;
        }

      /**
       * Block on upstream
       */
      if (cbuffer_len (&s->chars) < sizeof (char))
        {
          return;
        }

      scanner_execute (&s->s);
      parser_execute (&s->p);
    }
}
