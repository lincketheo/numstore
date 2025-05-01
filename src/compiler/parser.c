#include "compiler/parser.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/stack_parser.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/cbuffer.h"

DEFINE_DBG_ASSERT_I (parser, parser, p)
{
  ASSERT (p);
  ASSERT (p->tokens_input);
  ASSERT (p->queries_output);
}

err_t
parser_create (parser *dest, parser_params params)
{
  ASSERT (dest);

  err_t ret = SUCCESS;

  dest->queries_output = params.queries_output;
  dest->tokens_input = params.tokens_input;

  sp_params sparams = {
    .type_allocator = params.type_allocator,
    .stack_allocator = params.stack_allocator,
  };
  if ((ret = stackp_create (&dest->sp, sparams)))
    {
      return ret;
    }
  stackp_begin (&dest->sp, SBBT_QUERY);

  parser_assert (dest);

  return ret;
}

void
parser_execute (parser *p)
{
  parser_assert (p);

  // Block on downstream
  if (cbuffer_avail (p->queries_output) < sizeof (query))
    {
      return;
    }

  token tok;
  u32 read = cbuffer_read (&tok, sizeof tok, 1, p->tokens_input);
  ASSERT (read == 0 || read == 1);

  if (read > 0)
    {
      switch (stackp_feed_token (&p->sp, tok))
        {
        case SPR_DONE:
          {
            ast_result res = stackp_get (&p->sp);
            ASSERT (res.type == SBBT_QUERY);
            u32 written = cbuffer_write (
                &res.q, sizeof res.q, 1, p->queries_output);
            ASSERT (written == 1);
            stackp_begin (&p->sp, SBBT_QUERY);
            return;
          }
        case SPR_CONTINUE:
          {
            return;
          }
        case SPR_MALLOC_ERROR:
        case SPR_SYNTAX_ERROR:
          panic ();
        }
    }
}
