#include "compiler/stack_parser/queries/delete.h"

#include "dev/assert.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (delete_parser, delete_parser, s)
{
  ASSERT (s);
}

static const char *TAG = "Delete Parser";

static inline void
delete_parser_assert_state (delete_parser *q, int dltp_state)
{
  delete_parser_assert (q);
  ASSERT ((int)q->state == dltp_state);
}

#define HANDLER_FUNC(state) dltp_handle_##state

////////////////////////// API

delete_parser
dltp_create (delete_query *dest)
{
  delete_parser ret = {
    .state = DB_WAITING_FOR_VNAME,
    .builder = dltb_create (),
    .dest = dest,
  };

  delete_parser_assert_state (&ret, DB_WAITING_FOR_VNAME);

  return ret;
}

stackp_result
dltp_build (delete_parser *db, error *e)
{
  delete_parser_assert_state (db, DB_DONE);

  err_t ret = dltb_build (db->dest, &db->builder, e);

  switch (ret)
    {
    case ERR_INVALID_ARGUMENT:
      {
        return SPR_SYNTAX_ERROR;
      }
    case SUCCESS:
      {
        return SPR_DONE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (DB_WAITING_FOR_VNAME) (delete_parser *db, token t, error *e)
{
  delete_parser_assert_state (db, DB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return (stackp_result)error_causef (
          e, (err_t)SPR_SYNTAX_ERROR,
          "%s: Expecting token type %s, got %s",
          TAG, tt_tostr (TT_IDENTIFIER), tt_tostr (t.type));
    }

  switch (dltb_accept_string (&db->builder, t.str, e))
    {
    case ERR_INVALID_ARGUMENT:
      {
        return SPR_SYNTAX_ERROR;
      }
    case ERR_NOMEM:
      {
        return SPR_NOMEM;
      }
    case SUCCESS:
      {
        db->state = DB_DONE;
        return dltp_build (db, e);
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
dltp_accept_token (delete_parser *db, token t, error *e)
{
  delete_parser_assert (db);

  switch (db->state)
    {
    case DB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (DB_WAITING_FOR_VNAME) (db, t, e);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
