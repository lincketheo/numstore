#include "compiler/stack_parser/queries/insert_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_builder.h"
#include "compiler/tokens.h"
#include "dev/assert.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_builder, insert_builder, s)
{
  ASSERT (s->state == QB_INSERT);
  ASSERT (s);
}

static inline void
insert_builder_assert_state (query_builder *q, int ib_state)
{
  insert_builder_assert (q);
  ASSERT ((int)q->ib.state == ib_state);
}

#define HANDLER_FUNC(state) ib_handle_##state

////////////////////////// API

stackp_result
ib_insert (query_builder *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QB_UNKNOWN);

  dest->state = QB_INSERT;

  insert_builder_assert_state (dest, QB_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
ib_build (query_builder *ib)
{
  insert_builder_assert_state (ib, IB_DONE);
  ib->ret.iargs = (insert_args){
    .vname = ib->ib.vname,
  };
  ib->ret.type = QT_INSERT;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (IB_WAITING_FOR_VNAME) (query_builder *ib, token t)
{
  insert_builder_assert_state (ib, IB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  ib->ib.vname = t.str;
  ib->ib.state = IB_DONE;

  return SPR_DONE;
}

stackp_result
ib_accept_token (query_builder *ib, token t)
{
  insert_builder_assert (ib);

  switch (ib->ib.state)
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
