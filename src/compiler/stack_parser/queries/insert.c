#include "compiler/stack_parser/queries/insert.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_parser.h"
#include "compiler/tokens.h"
#include "dev/assert.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_parser, insert_parser, s)
{
  ASSERT (s->state == QP_INSERT);
  ASSERT (s);
}

static inline void
insert_parser_assert_state (query_parser *q, int inp_state)
{
  insert_parser_assert (q);
  ASSERT ((int)q->ip.state == inp_state);
}

#define HANDLER_FUNC(state) inp_handle_##state

////////////////////////// API

stackp_result
inp_insert (query_parser *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QP_UNKNOWN);

  dest->state = QP_INSERT;

  insert_parser_assert_state (dest, QP_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
inp_build (query_parser *ib)
{
  insert_parser_assert_state (ib, IB_DONE);
  /**
  ib->ret.iquery = (insert_query){
    .vname = ib->ip.vname,
  };
  */
  ib->ret.type = QT_INSERT;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (IB_WAITING_FOR_VNAME) (query_parser *ib, token t)
{
  insert_parser_assert_state (ib, IB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  // ib->ip.vname = t.str;
  ib->ip.state = IB_DONE;

  return SPR_DONE;
}

stackp_result
inp_accept_token (query_parser *ib, token t)
{
  insert_parser_assert (ib);

  switch (ib->ip.state)
    {
    case IB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (IB_WAITING_FOR_VNAME) (ib, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
