#include "compiler/stack_parser/queries/update.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_parser.h"
#include "compiler/tokens.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_parser, update_parser, s)
{
  ASSERT (s->state == QP_TAKE);
  ASSERT (s);
}

static inline void
update_parser_assert_state (query_parser *q, int updtp_state)
{
  update_parser_assert (q);
  ASSERT ((int)q->up.state == updtp_state);
}

#define HANDLER_FUNC(state) updtp_handle_##state

////////////////////////// API

stackp_result
updtp_update (query_parser *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QP_UNKNOWN);

  dest->state = QP_TAKE;

  update_parser_assert_state (dest, QP_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
updtp_build (query_parser *updb)
{
  update_parser_assert_state (updb, UPDB_DONE);
  /**
  updb->ret.uquery = (update_query){
    .vname = updb->up.vname,
  };
  */
  updb->ret.type = QT_TAKE;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (UPDB_WAITING_FOR_VNAME) (query_parser *updb, token t)
{
  update_parser_assert_state (updb, UPDB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  // updb->up.vname = t.str;
  updb->up.state = UPDB_DONE;

  return SPR_DONE;
}

stackp_result
updtp_accept_token (query_parser *updb, token t)
{
  update_parser_assert (updb);

  switch (updb->up.state)
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
