#include "compiler/stack_parser/queries/delete_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_builder.h"
#include "compiler/tokens.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_builder, delete_builder, s)
{
  ASSERT (s->state == QB_DELETE);
  ASSERT (s);
}

static inline void
delete_builder_assert_state (query_builder *q, int db_state)
{
  delete_builder_assert (q);
  ASSERT ((int)q->db.state == db_state);
}

#define HANDLER_FUNC(state) db_handle_##state

////////////////////////// API

stackp_result
db_delete (query_builder *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QB_UNKNOWN);

  dest->state = QB_DELETE;

  delete_builder_assert_state (dest, QB_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
db_build (query_builder *db)
{
  delete_builder_assert_state (db, DB_DONE);
  db->ret.dquery = (delete_query){
    .vname = db->db.vname,
  };
  db->ret.type = QT_DELETE;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (DB_WAITING_FOR_VNAME) (query_builder *db, token t)
{
  delete_builder_assert_state (db, DB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  db->db.vname = t.str;
  db->db.state = DB_DONE;

  return SPR_DONE;
}

stackp_result
db_accept_token (query_builder *db, token t)
{
  delete_builder_assert (db);

  switch (db->db.state)
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
