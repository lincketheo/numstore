#include "compiler/stack_parser/queries/take_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_builder.h"
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

stackp_result
tb_take (query_builder *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QB_UNKNOWN);

  dest->state = QB_TAKE;

  take_builder_assert_state (dest, QB_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
tb_build (query_builder *tb)
{
  take_builder_assert_state (tb, TB_DONE);
  tb->ret.tquery = (take_query){
    .vname = tb->tb.vname,
  };
  tb->ret.type = QT_TAKE;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (TB_WAITING_FOR_VNAME) (query_builder *tb, token t)
{
  take_builder_assert_state (tb, TB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  tb->tb.vname = t.str;
  tb->tb.state = TB_DONE;

  return SPR_DONE;
}

stackp_result
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
        return SPR_SYNTAX_ERROR;
      }
    }
}
