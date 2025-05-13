#include "compiler/parser.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/stack_parser.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "intf/logging.h"

DEFINE_DBG_ASSERT_I (parser, parser, p)
{
  ASSERT (p);
  ASSERT (p->tokens_input);
  ASSERT (p->queries_output);
}

parser
parser_create (parser_params params)
{
  parser ret = {
    .sp = stackp_create ((sp_params){
        .type_allocator = params.type_allocator,
    }),
    .queries_output = params.queries_output,
    .tokens_input = params.tokens_input,
    .is_error = false,
  };

  stackp_begin (&ret.sp, SBBT_QUERY);

  parser_assert (&ret);

  return ret;
}

static inline void
parser_write_out (parser *p)
{
  ast_result res = stackp_get (&p->sp);

  ASSERT (res.type == SBBT_QUERY);
  u32 written = cbuffer_write (
      &res.q,
      sizeof res.q,
      1, p->queries_output);
  ASSERT (written == 1);

  return;
}

void
parser_execute (parser *p)
{
  parser_assert (p);

  while (true)
    {
      // Block on downstream
      if (cbuffer_avail (p->queries_output) < sizeof (query))
        {
          return;
        }

      token tok;
      u32 read = cbuffer_read (&tok, sizeof tok, 1, p->tokens_input);

      // Block on upstream
      if (read == 0)
        {
          return;
        }

      string tstr = tt_tostr (tok.type);
      i_log_trace ("Parser got token: %.*s\n", tstr.len, tstr.data);

      /**
       * Listen to upstream errors
       */
      if (tok.type == TT_ERROR)
        {
          stackp_reset (&p->sp);
          stackp_begin (&p->sp, SBBT_QUERY);
          p->is_error = true;
          continue;
        }

      if (p->is_error)
        {
          switch (tok.type)
            {
            case TT_CREATE:
            case TT_DELETE:
            case TT_APPEND:
            case TT_READ:
            case TT_INSERT:
            case TT_UPDATE:
            case TT_TAKE:
              {
                p->is_error = false;
                break;
              }
            default:
              {
                continue;
              }
            }
        }

      if (!p->is_error)
        {
          switch (stackp_feed_token (&p->sp, tok))
            {
            case SPR_DONE:
              {
                parser_write_out (p);
                stackp_begin (&p->sp, SBBT_QUERY);
                break;
              }
            case SPR_CONTINUE:
              {
                continue; // Do nothing
              }
            case SPR_NOMEM:
            case SPR_STACK_OVERFLOW:
            case SPR_SYNTAX_ERROR:
              {
                stackp_reset (&p->sp);
                stackp_begin (&p->sp, SBBT_QUERY);
                p->is_error = true;
                break;
              }
            }
        }
    }
}
