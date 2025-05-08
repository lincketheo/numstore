#include "query/queries/append.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_parser.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/logging.h"
#include "services/var_retr.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_parser, append_parser, s)
{
  ASSERT (s->state == QP_APPEND);
  ASSERT (s);
}

static inline void
append_parser_assert_state (query_parser *q, int apnd_state)
{
  append_parser_assert (q);
  ASSERT ((int)q->ap.state == apnd_state);
}

#define HANDLER_FUNC(state) apnd_handle_##state

////////////////////////// API

stackp_result
apnd_create (query_parser *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QP_UNKNOWN);

  dest->state = QP_APPEND;

  append_parser_assert_state (dest, QP_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
apnd_build (query_parser *ab)
{
  append_parser_assert_state (ab, AB_DONE);
  /**
  ab->ret.aquery = (append_query){
    .var = ab->ap.variable,
  };
  */
  panic ();
  ab->ret.type = QT_APPEND;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (AB_WAITING_FOR_VNAME) (query_parser *ab, token t)
{
  append_parser_assert_state (ab, AB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  ab->ap.state = AB_DONE;

  return SPR_DONE;
}

stackp_result
apnd_accept_token (query_parser *ab, token t)
{
  append_parser_assert (ab);

  switch (ab->ap.state)
    {
    case AB_WAITING_FOR_VNAME:
      {
        return HANDLER_FUNC (AB_WAITING_FOR_VNAME) (ab, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
