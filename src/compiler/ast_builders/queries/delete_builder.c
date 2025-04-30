#include "compiler/ast_builders/queries/delete_builder.h"
#include "compiler/ast_builders/common.h"
#include "compiler/ast_builders/query_builder.h"
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

parse_result
db_delete (query_builder *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QB_UNKNOWN);

  dest->state = QB_DELETE;

  delete_builder_assert_state (dest, QB_UNKNOWN);

  return PR_CONTINUE;
}

parse_result
db_build (query_builder *db)
{
  delete_builder_assert_state (db, DB_DONE);
  db->ret.dargs = (delete_args){
    .vname = db->db.vname,
  };
  db->ret.type = QT_DELETE;
  return PR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline parse_result
HANDLER_FUNC (DB_WAITING_FOR_VNAME) (query_builder *db, token t)
{
  delete_builder_assert_state (db, DB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return PR_SYNTAX_ERROR;
    }

  db->db.vname = t.str;
  db->db.state = DB_DONE;

  return PR_DONE;
}

parse_result
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
        return PR_SYNTAX_ERROR;
      }
    }
}
