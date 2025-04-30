#include "compiler/ast_builders/queries/create_builder.h"
#include "compiler/ast_builders/common.h"
#include "compiler/ast_builders/query_builder.h"
#include "compiler/tokens.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_builder, create_builder, s)
{
  ASSERT (s->state == QB_CREATE);
  ASSERT (s);
}

static inline void
create_builder_assert_state (query_builder *q, int cb_state)
{
  create_builder_assert (q);
  ASSERT ((int)q->cb.state == cb_state);
}

#define HANDLER_FUNC(state) cb_handle_##state

////////////////////////// API

parse_result
cb_create (query_builder *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QB_UNKNOWN);

  dest->state = QB_CREATE;

  create_builder_assert_state (dest, QB_UNKNOWN);

  return PR_CONTINUE;
}

parse_result
cb_build (query_builder *cb)
{
  create_builder_assert_state (cb, CB_DONE);
  cb->ret.cargs = (create_args){
    .type = cb->cb.type,
    .vname = cb->cb.vname,
  };
  cb->ret.type = QT_CREATE;
  return PR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline parse_result
HANDLER_FUNC (CB_WAITING_FOR_VNAME) (query_builder *cb, token t)
{
  create_builder_assert_state (cb, CB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return PR_SYNTAX_ERROR;
    }

  cb->cb.vname = t.str;
  cb->cb.state = CB_WAITING_FOR_TYPE;

  return PR_CONTINUE;
}

parse_result
cb_accept_token (query_builder *cb, token t)
{
  create_builder_assert (cb);

  switch (cb->cb.state)
    {
    case CB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (CB_WAITING_FOR_VNAME) (cb, t);
      }

    default:
      {
        return PR_SYNTAX_ERROR;
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline parse_result
HANDLER_FUNC (CB_WAITING_FOR_TYPE) (query_builder *cb, type t)
{
  create_builder_assert_state (cb, CB_WAITING_FOR_TYPE);
  cb->cb.type = t;
  cb->cb.state = CB_DONE;
  return PR_DONE;
}

parse_result
cb_accept_type (query_builder *cb, type type)
{
  create_builder_assert (cb);

  switch (cb->cb.state)
    {
    case CB_WAITING_FOR_TYPE:
      {
        return HANDLER_FUNC (CB_WAITING_FOR_TYPE) (cb, type);
      }

    default:
      {
        return PR_SYNTAX_ERROR;
      }
    }
}
