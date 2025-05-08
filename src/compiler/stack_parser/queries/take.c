#include "compiler/stack_parser/queries/take.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_parser.h"
#include "compiler/tokens.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_parser, take_parser, s)
{
  ASSERT (s->state == QP_TAKE);
  ASSERT (s);
}

static inline void
take_parser_assert_state (query_parser *q, int tkp_state)
{
  take_parser_assert (q);
  ASSERT ((int)q->tp.state == tkp_state);
}

#define HANDLER_FUNC(state) tkp_handle_##state

////////////////////////// API

stackp_result
tkp_take (query_parser *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QP_UNKNOWN);

  dest->state = QP_TAKE;

  take_parser_assert_state (dest, QP_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
tkp_build (query_parser *tb)
{
  take_parser_assert_state (tb, TB_DONE);
  /**
  tb->ret.tquery = (take_query){
    .vname = tb->tp.vname,
  };
  */
  tb->ret.type = QT_TAKE;

  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (TB_WAITING_FOR_VNAME) (query_parser *tb, token t)
{
  take_parser_assert_state (tb, TB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  // tb->tp.vname = t.str;
  tb->tp.state = TB_DONE;

  return SPR_DONE;
}

stackp_result
tkp_accept_token (query_parser *tb, token t)
{
  take_parser_assert (tb);

  switch (tb->tp.state)
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
