#include "compiler/stack_parser/types/enum.h"

#include "ast/type/builders/enum.h" // enum_builder
#include "dev/assert.h"             // DEFINE_DBG_ASSERT_I

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (enum_parser, enum_parser, s)
{
  ASSERT (s);
}

static const char *TAG = "Enum Parser";

static inline void
enum_parser_assert_state (enum_parser *tb, int enp_state)
{
  enum_parser_assert (tb);
  ASSERT ((int)tb->state == enp_state);
}

#define HANDLER_FUNC(state) enp_handle_##state

////////////////////////// API

enum_parser
enp_create (lalloc *alloc)
{
  ASSERT (alloc);

  enum_parser ret = {
    .state = ENP_WAITING_FOR_LB,
    .builder = enb_create (alloc),
  };
  enum_parser_assert_state (&ret, ENP_WAITING_FOR_LB);
  return ret;
}

stackp_result
enp_build (enum_t *dest, enum_parser *eb, lalloc *destination, error *e)
{
  enum_parser_assert_state (eb, ENP_DONE);

  switch (enb_build (dest, &eb->builder, destination, e))
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
HANDLER_FUNC (ENP_WAITING_FOR_LB) (enum_parser *eb, token t, error *e)
{
  enum_parser_assert_state (eb, ENP_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return (stackp_result)error_causef (
          e, (err_t)SPR_SYNTAX_ERROR,
          "%s: Expecting token type %s, got %s",
          TAG, tt_tostr (TT_LEFT_BRACE), tt_tostr (t.type));
    }

  eb->state = ENP_WAITING_FOR_IDENT;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (ENP_WAITING_FOR_IDENT) (
    enum_parser *eb, token t, error *e)
{
  enum_parser_assert_state (eb, ENP_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  switch (enb_accept_key (&eb->builder, t.str, e))
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
        eb->state = ENP_WAITING_FOR_COMMA_OR_RIGHT;
        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline stackp_result
HANDLER_FUNC (ENP_WAITING_FOR_COMMA_OR_RIGHT) (enum_parser *eb, token t, error *e)
{
  enum_parser_assert_state (eb, ENP_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      eb->state = ENP_WAITING_FOR_IDENT;
      return SPR_CONTINUE;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      eb->state = ENP_DONE;
      return SPR_DONE;
    }

  return (stackp_result)error_causef (
      e, (err_t)SPR_SYNTAX_ERROR,
      "%s: Expected token type: %s or %s, got %s",
      TAG, tt_tostr (TT_COMMA),
      tt_tostr (TT_RIGHT_BRACE),
      tt_tostr (t.type));
}

stackp_result
enp_accept_token (enum_parser *eb, token t, error *e)
{
  enum_parser_assert (eb);

  switch (eb->state)
    {
    case ENP_WAITING_FOR_LB:
      {
        return HANDLER_FUNC (ENP_WAITING_FOR_LB) (eb, t, e);
      }

    case ENP_WAITING_FOR_IDENT:
      {
        return HANDLER_FUNC (ENP_WAITING_FOR_IDENT) (eb, t, e);
      }

    case ENP_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return HANDLER_FUNC (ENP_WAITING_FOR_COMMA_OR_RIGHT) (eb, t, e);
      }

    default:
      {
        UNREACHABLE ();
      }
    }
}
