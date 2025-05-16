#include "compiler/compiler.h"

void
compiler_create (
    compiler *dest,
    cbuffer *chars_input,
    cbuffer *queries_output,
    qspce_prvdr *qspcp)
{
  ASSERT (dest);
  dest->chars_input = chars_input;
  dest->tokens = cbuffer_create_from (dest->_tokens);
  dest->query_output = queries_output;

  dest->scanner = scanner_create (
      dest->chars_input,
      &dest->tokens,
      qspcp);
  parser_create (&dest->parser, &dest->tokens, dest->query_output);
}

bool
compiler_done (compiler *c)
{
  return cbuffer_len (c->chars_input) + cbuffer_len (&c->tokens) == 0;
}

err_t
compiler_execute (compiler *c, error *e)
{
  for (int i = 0; i < 10; ++i)
    {
      err_t_wrap (scanner_execute (&c->scanner, e), e);
      err_t_wrap (parser_execute (&c->parser, e), e);
    }
  return SUCCESS;
}
