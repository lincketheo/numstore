#include "compiler/stack_parser/queries/delete.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_parser.h"
#include "compiler/tokens.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_parser, delete_parser, s)
{
  ASSERT (s->state == QP_DELETE);
  ASSERT (s);
}

static inline void
delete_parser_assert_state (query_parser *q, int dltp_state)
{
  delete_parser_assert (q);
  ASSERT ((int)q->dp.state == dltp_state);
}

#define HANDLER_FUNC(state) dltp_handle_##state

////////////////////////// API

stackp_result
dltp_delete (query_parser *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QP_UNKNOWN);

  dest->state = QP_DELETE;

  delete_parser_assert_state (dest, QP_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
dltp_build (query_parser *db)
{
  delete_parser_assert_state (db, DB_DONE);
  /**
  db->ret.dquery = (delete_query){
    .vname = db->dp.vname,
  };
  */
  db->ret.type = QT_DELETE;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (DB_WAITING_FOR_VNAME) (query_parser *db, token t)
{
  delete_parser_assert_state (db, DB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  // db->dp.vname = t.str;
  db->dp.state = DB_DONE;

  return SPR_DONE;
}

stackp_result
dltp_accept_token (query_parser *db, token t)
{
  delete_parser_assert (db);

  switch (db->dp.state)
    {
    case DB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (DB_WAITING_FOR_VNAME) (db, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
