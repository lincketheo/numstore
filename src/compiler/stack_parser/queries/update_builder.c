#include "compiler/stack_parser/queries/update_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_builder.h"
#include "compiler/tokens.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_builder, update_builder, s)
{
  ASSERT (s->state == QB_TAKE);
  ASSERT (s);
}

static inline void
update_builder_assert_state (query_builder *q, int updb_state)
{
  update_builder_assert (q);
  ASSERT ((int)q->ub.state == updb_state);
}

#define HANDLER_FUNC(state) updb_handle_##state

////////////////////////// API

stackp_result
updb_update (query_builder *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QB_UNKNOWN);

  dest->state = QB_TAKE;

  update_builder_assert_state (dest, QB_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
updb_build (query_builder *updb)
{
  update_builder_assert_state (updb, UPDB_DONE);
  updb->ret.uargs = (update_args){
    .vname = updb->ub.vname,
  };
  updb->ret.type = QT_TAKE;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (UPDB_WAITING_FOR_VNAME) (query_builder *updb, token t)
{
  update_builder_assert_state (updb, UPDB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  updb->ub.vname = t.str;
  updb->ub.state = UPDB_DONE;

  return SPR_DONE;
}

stackp_result
updb_accept_token (query_builder *updb, token t)
{
  update_builder_assert (updb);

  switch (updb->ub.state)
    {
    case UPDB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (UPDB_WAITING_FOR_VNAME) (updb, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
