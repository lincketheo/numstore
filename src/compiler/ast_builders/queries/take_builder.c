#include "compiler/ast_builders/queries/take_builder.h"
#include "compiler/ast_builders/common.h"
#include "compiler/ast_builders/query_builder.h"
#include "compiler/tokens.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_builder, take_builder, s)
{
  ASSERT (s->state == QB_TAKE);
  ASSERT (s);
}

static inline void
take_builder_assert_state (query_builder *q, int tb_state)
{
  take_builder_assert (q);
  ASSERT ((int)q->tb.state == tb_state);
}

#define HANDLER_FUNC(state) tb_handle_##state

////////////////////////// API

parse_result
tb_take (query_builder *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QB_UNKNOWN);

  dest->state = QB_TAKE;

  take_builder_assert_state (dest, QB_UNKNOWN);

  return PR_CONTINUE;
}

parse_result
tb_build (query_builder *tb)
{
  take_builder_assert_state (tb, TB_DONE);
  tb->ret.targs = (take_args){
    .vname = tb->tb.vname,
  };
  tb->ret.type = QT_TAKE;
  return PR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline parse_result
HANDLER_FUNC (TB_WAITING_FOR_VNAME) (query_builder *tb, token t)
{
  take_builder_assert_state (tb, TB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return PR_SYNTAX_ERROR;
    }

  tb->tb.vname = t.str;
  tb->tb.state = TB_DONE;

  return PR_DONE;
}

parse_result
tb_accept_token (query_builder *tb, token t)
{
  take_builder_assert (tb);

  switch (tb->tb.state)
    {
    case TB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (TB_WAITING_FOR_VNAME) (tb, t);
      }

    default:
      {
        return PR_SYNTAX_ERROR;
      }
    }
}
