#include "compiler/stack_parser/queries/create.h"

#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_parser.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "query/builders/create.h"
#include "query/queries/create.h"
#include "query/query.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_parser, create_parser, s)
{
  ASSERT (s->state == QP_CREATE);
  ASSERT (s);
}

static inline void
create_parser_assert_state (query_parser *q, int crtp_state)
{
  create_parser_assert (q);
  ASSERT ((int)q->cp.state == crtp_state);
}

#define HANDLER_FUNC(state) crtp_handle_##state

////////////////////////// API

stackp_result
crtp_create (query_parser *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QP_UNKNOWN);

  dest->state = QP_CREATE;

  create_parser_assert_state (dest, QP_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
crtp_build (query_parser *cb)
{
  create_parser_assert_state (cb, CB_DONE);

  error e = error_create (NULL);
  switch (crb_build (&cb->ret.cquery, &cb->cp.builder, &e))
    {
    case ERR_INVALID_ARGUMENT:
      {
        return SPR_SYNTAX_ERROR;
      }
    case SUCCESS:
      {
        cb->ret.type = QT_CREATE;
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
HANDLER_FUNC (CB_WAITING_FOR_VNAME) (query_parser *cb, token t)
{
  create_parser_assert_state (cb, CB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  error e = error_create (NULL);
  switch (crb_accept_string (&cb->cp.builder, t.str, &e))
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
        cb->cp.state = CB_WAITING_FOR_TYPE;
        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
crtp_accept_token (query_parser *cb, token t)
{
  create_parser_assert (cb);

  switch (cb->cp.state)
    {
    case CB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (CB_WAITING_FOR_VNAME) (cb, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
HANDLER_FUNC (CB_WAITING_FOR_TYPE) (query_parser *cb, type t)
{
  (void)t;
  create_parser_assert_state (cb, CB_WAITING_FOR_TYPE);

  error e = error_create (NULL);
  switch (crb_accept_type (&cb->cp.builder, t, &e))
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
        cb->cp.state = CB_DONE;
        return SPR_DONE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
crtp_accept_type (query_parser *cb, type type)
{
  create_parser_assert (cb);

  switch (cb->cp.state)
    {
    case CB_WAITING_FOR_TYPE:
      {
        return HANDLER_FUNC (CB_WAITING_FOR_TYPE) (cb, type);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
