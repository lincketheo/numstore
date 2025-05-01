#include "compiler/stack_parser/queries/append_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_builder.h"
#include "compiler/tokens.h"
#include "dev/assert.h"

////////////////////////// DEV
DEFINE_DBG_ASSERT_I (query_builder, append_builder, s)
{
  ASSERT (s->state == QB_APPEND);
  ASSERT (s);
}

static inline void
append_builder_assert_state (query_builder *q, int ab_state)
{
  append_builder_assert (q);
  ASSERT ((int)q->ab.state == ab_state);
}

#define HANDLER_FUNC(state) ab_handle_##state

////////////////////////// API

stackp_result
ab_append (query_builder *dest)
{
  ASSERT (dest);
  ASSERT (dest->state == QB_UNKNOWN);

  dest->state = QB_APPEND;

  append_builder_assert_state (dest, QB_UNKNOWN);

  return SPR_CONTINUE;
}

stackp_result
ab_build (query_builder *ab)
{
  append_builder_assert_state (ab, AB_DONE);
  ab->ret.aargs = (append_args){
    .vname = ab->ab.vname,
  };
  ab->ret.type = QT_APPEND;
  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (AB_WAITING_FOR_VNAME) (query_builder *ab, token t)
{
  append_builder_assert_state (ab, AB_WAITING_FOR_VNAME);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  ab->ab.vname = t.str;
  ab->ab.state = AB_DONE;

  return SPR_DONE;
}

stackp_result
ab_accept_token (query_builder *ab, token t)
{
  append_builder_assert (ab);

  switch (ab->ab.state)
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
