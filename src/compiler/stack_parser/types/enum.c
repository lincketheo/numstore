#include "compiler/stack_parser/types/enum.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/type_parser.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "type/builders/enum.h"
#include "type/types.h"
#include "utils/bounds.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (type_parser, enum_parser, s)
{
  ASSERT (s);
}

static inline void
enum_parser_assert_state (type_parser *tb, int enp_state)
{
  enum_parser_assert (tb);
  ASSERT ((int)tb->enp.state == enp_state);
}

#define HANDLER_FUNC(state) enp_handle_##state

////////////////////////// API

stackp_result
enp_create (type_parser *dest, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->state == TB_UNKNOWN);

  error e = error_create (NULL);
  switch (enb_create (&dest->enp.builder, alloc, &e))
    {
    case ERR_NOMEM:
      {
        return SPR_NOMEM;
      }
    case SUCCESS:
      {
        dest->enp.state = ENP_WAITING_FOR_LB;
        dest->state = TB_ENUM;

        enum_parser_assert (dest);

        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
enp_build (type_parser *eb)
{
  enum_parser_assert_state (eb, ENP_DONE);

  error e = error_create (NULL);
  switch (enb_build (&eb->ret.en, &eb->enp.builder, &e))
    {
    case ERR_INVALID_TYPE:
      {
        return SPR_SYNTAX_ERROR;
      }
    case ERR_NOMEM:
      {
        return SPR_NOMEM;
      }
    case SUCCESS:
      {
        eb->ret.type = T_ENUM;
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
HANDLER_FUNC (ENP_WAITING_FOR_LB) (type_parser *eb, token t)
{
  enum_parser_assert_state (eb, ENP_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return SPR_SYNTAX_ERROR;
    }

  eb->enp.state = ENP_WAITING_FOR_IDENT;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (ENP_WAITING_FOR_IDENT) (
    type_parser *eb, token t)
{
  enum_parser_assert_state (eb, ENP_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  error e = error_create (NULL);
  switch (enb_accept_key (&eb->enp.builder, t.str, &e))
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
        eb->enp.state = ENP_WAITING_FOR_COMMA_OR_RIGHT;
        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline stackp_result
HANDLER_FUNC (ENP_WAITING_FOR_COMMA_OR_RIGHT) (type_parser *eb, token t)
{
  enum_parser_assert_state (
      eb, ENP_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      eb->enp.state = ENP_WAITING_FOR_IDENT;
      return SPR_CONTINUE;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      eb->enp.state = ENP_DONE;
      return SPR_DONE;
    }

  return SPR_SYNTAX_ERROR;
}

stackp_result
enp_accept_token (type_parser *eb, token t)
{
  enum_parser_assert (eb);
  ASSERT (eb->state == TB_ENUM);

  switch (eb->enp.state)
    {
    case ENP_WAITING_FOR_LB:
      {
        return HANDLER_FUNC (ENP_WAITING_FOR_LB) (eb, t);
      }

    case ENP_WAITING_FOR_IDENT:
      {
        return HANDLER_FUNC (ENP_WAITING_FOR_IDENT) (eb, t);
      }

    case ENP_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return HANDLER_FUNC (ENP_WAITING_FOR_COMMA_OR_RIGHT) (eb, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
