#include "compiler/stack_parser/queries/read.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_parser.h"
#include "compiler/tokens.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_parser, read_parser, s)
{
  ASSERT (s->state == QP_READ);
  ASSERT (s);
}

static inline void
read_parser_assert_state (query_parser *q, int rdp_state)
{
  read_parser_assert (q);
  ASSERT ((int)q->rp.state == rdp_state);
}

#define HANDLER_FUNC(state) rdp_handle_##state

////////////////////////// API

stackp_result
rdp_read (query_parser *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QP_UNKNOWN);

  dest->state = QP_READ;

  read_parser_assert_state (dest, QP_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
rdp_build (query_parser *rb)
{
  read_parser_assert_state (rb, RB_DONE);
  /**
  rb->ret.rquery = (read_query){
    .vname = rb->rp.vname,
  };
  */
  rb->ret.type = QT_READ;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (RB_WAITING_FOR_VNAME) (query_parser *rb, token t)
{
  read_parser_assert_state (rb, RB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  // rb->rp.vname = t.str;
  rb->rp.state = RB_DONE;

  return SPR_DONE;
}

stackp_result
rdp_accept_token (query_parser *rb, token t)
{
  read_parser_assert (rb);

  switch (rb->rp.state)
    {
    case RB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (RB_WAITING_FOR_VNAME) (rb, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
