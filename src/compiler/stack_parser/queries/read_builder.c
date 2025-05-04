#include "compiler/stack_parser/queries/read_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_builder.h"
#include "compiler/tokens.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_builder, read_builder, s)
{
  ASSERT (s->state == QB_READ);
  ASSERT (s);
}

static inline void
read_builder_assert_state (query_builder *q, int rb_state)
{
  read_builder_assert (q);
  ASSERT ((int)q->rb.state == rb_state);
}

#define HANDLER_FUNC(state) rb_handle_##state

////////////////////////// API

stackp_result
rb_read (query_builder *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QB_UNKNOWN);

  dest->state = QB_READ;

  read_builder_assert_state (dest, QB_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
rb_build (query_builder *rb)
{
  read_builder_assert_state (rb, RB_DONE);
  rb->ret.rquery = (read_query){
    .vname = rb->rb.vname,
  };
  rb->ret.type = QT_READ;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (RB_WAITING_FOR_VNAME) (query_builder *rb, token t)
{
  read_builder_assert_state (rb, RB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  rb->rb.vname = t.str;
  rb->rb.state = RB_DONE;

  return SPR_DONE;
}

stackp_result
rb_accept_token (query_builder *rb, token t)
{
  read_builder_assert (rb);

  switch (rb->rb.state)
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
