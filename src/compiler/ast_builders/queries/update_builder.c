#include "compiler/ast_builders/queries/update_builder.h"
#include "compiler/ast_builders/common.h"
#include "compiler/ast_builders/query_builder.h"
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

parse_result
updb_update (query_builder *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QB_UNKNOWN);

  dest->state = QB_TAKE;

  update_builder_assert_state (dest, QB_UNKNOWN);

  return PR_CONTINUE;
}

parse_result
updb_build (query_builder *updb)
{
  update_builder_assert_state (updb, UB_DONE);
  updb->ret.uargs = (update_args){
    .vname = updb->ub.vname,
  };
  updb->ret.type = QT_TAKE;
  return PR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline parse_result
HANDLER_FUNC (UB_WAITING_FOR_VNAME) (query_builder *updb, token t)
{
  update_builder_assert_state (updb, UB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return PR_SYNTAX_ERROR;
    }

  updb->ub.vname = t.str;
  updb->ub.state = UB_DONE;

  return PR_DONE;
}

parse_result
updb_accept_token (query_builder *updb, token t)
{
  update_builder_assert (updb);

  switch (updb->ub.state)
    {
    case UB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (UB_WAITING_FOR_VNAME) (updb, t);
      }

    default:
      {
        return PR_SYNTAX_ERROR;
      }
    }
}
