#include "compiler/stack_parser/queries/create.h"

#include "dev/assert.h" // DEFINE_DBG_ASSERT_I

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (create_parser, create_parser, s)
{
  ASSERT (s);
}

static const char *TAG = "Create Parser";

static inline void
create_parser_assert_state (create_parser *q, int crtp_state)
{
  create_parser_assert (q);
  ASSERT ((int)q->state == crtp_state);
}

#define HANDLER_FUNC(state) crtp_handle_##state

////////////////////////// API

create_parser
crtp_create (void)
{
  create_parser ret = {
    .builder = crb_create (),
    .state = CB_WAITING_FOR_VNAME,
  };

  create_parser_assert_state (&ret, CB_WAITING_FOR_VNAME);

  return ret;
}

stackp_result
crtp_build (create_query *dest, create_parser *cb, error *e)
{
  create_parser_assert_state (cb, CB_DONE);

  switch (crb_build (dest, &cb->builder, e))
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
HANDLER_FUNC (CB_WAITING_FOR_VNAME) (create_parser *cb, token t, error *e)
{
  create_parser_assert_state (cb, CB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return (stackp_result)error_causef (
          e, (err_t)SPR_SYNTAX_ERROR,
          "%s: Expecting token type %s, got %s",
          TAG, tt_tostr (TT_IDENTIFIER), tt_tostr (t.type));
    }

  switch (crb_accept_string (&cb->builder, t.str, e))
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
        cb->state = CB_WAITING_FOR_TYPE;
        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
crtp_accept_token (create_parser *cb, token t, error *e)
{
  create_parser_assert (cb);

  switch (cb->state)
    {
    case CB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (CB_WAITING_FOR_VNAME) (cb, t, e);
      }

    default:
      {
        UNREACHABLE ();
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
HANDLER_FUNC (CB_WAITING_FOR_TYPE) (create_parser *cb, type t, error *e)
{
  create_parser_assert_state (cb, CB_WAITING_FOR_TYPE);

  switch (crb_accept_type (&cb->builder, t, e))
    {
    case ERR_NOMEM:
      {
        return SPR_NOMEM;
      }
    case ERR_INVALID_ARGUMENT:
      {
        return SPR_SYNTAX_ERROR;
      }
    case SUCCESS:
      {
        cb->state = CB_DONE;
        return SPR_DONE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
crtp_accept_type (create_parser *cb, type type, error *e)
{
  create_parser_assert (cb);

  switch (cb->state)
    {
    case CB_WAITING_FOR_TYPE:
      {
        return HANDLER_FUNC (CB_WAITING_FOR_TYPE) (cb, type, e);
      }

    default:
      {
        UNREACHABLE ();
      }
    }
}
