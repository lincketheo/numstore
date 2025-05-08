#include "compiler/stack_parser/types/union.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/type_parser.h"
#include "compiler/tokens.h"
#include "type/types.h"
#include "utils/bounds.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (type_parser, union_parser, s)
{
  ASSERT (s);
  ASSERT (s->state == TB_UNION);
}

static inline void
union_parser_assert_state (type_parser *unp, int unp_state)
{
  union_parser_assert (unp);
  ASSERT ((int)unp->unp.state == unp_state);
}

#define HANDLER_FUNC(state) unp_handle_##state

////////////////////////// API

stackp_result
unp_create (type_parser *dest, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->state == TB_UNKNOWN);

  switch (unb_create (&dest->unp.builder, alloc))
    {
    case ERR_NOMEM:
      {
        return SPR_MALLOC_ERROR;
      }
    case SUCCESS:
      {
        dest->unp.state = UNP_WAITING_FOR_LB;
        dest->state = TB_UNION;

        union_parser_assert_state (
            dest, UNP_WAITING_FOR_LB);

        return SPR_CONTINUE;
      }
    default:
      {
        ASSERT (0);
        return SPR_MALLOC_ERROR;
      }
    }
}

stackp_result
unp_build (type_parser *unp)
{
  union_parser_assert_state (unp, UNP_DONE);

  switch (unb_build (&unp->ret.un, &unp->unp.builder))
    {
    case ERR_INVALID_ARGUMENT:
      {
        return SPR_SYNTAX_ERROR;
      }
    case ERR_IO:
      {
        return SPR_MALLOC_ERROR;
      }
    case SUCCESS:
      {
        unp->ret.type = T_UNION;
        return SPR_DONE;
      }
    default:
      {
        ASSERT (0);
        return SPR_MALLOC_ERROR;
      }
    }

  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (UNP_WAITING_FOR_LB) (
    type_parser *unp, token t)
{
  union_parser_assert_state (unp, UNP_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return SPR_SYNTAX_ERROR;
    }

  unp->unp.state = UNP_WAITING_FOR_IDENT;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (UNP_WAITING_FOR_IDENT) (
    type_parser *unp, token t)
{
  union_parser_assert_state (unp, UNP_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  switch (unb_accept_key (&unp->unp.builder, t.str))
    {
    case ERR_INVALID_ARGUMENT:
      {
        return SPR_SYNTAX_ERROR;
      }
    case ERR_NOMEM:
      {
        return SPR_MALLOC_ERROR;
      }
    case SUCCESS:
      {
        unp->unp.state = UNP_WAITING_FOR_TYPE;
        return SPR_CONTINUE;
      }
    default:
      {
        ASSERT (0);
        return SPR_MALLOC_ERROR;
      }
    }

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (UNP_WAITING_FOR_COMMA_OR_RIGHT) (
    type_parser *unp, token t)
{
  union_parser_assert_state (
      unp, UNP_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      unp->unp.state = UNP_WAITING_FOR_IDENT;
      return SPR_CONTINUE;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      unp->unp.state = UNP_DONE;
      return SPR_DONE;
    }

  return SPR_SYNTAX_ERROR;
}

stackp_result
unp_accept_token (type_parser *unp, token t)
{
  union_parser_assert (unp);
  ASSERT (unp->state == TB_UNION);

  switch (unp->unp.state)
    {

    case UNP_WAITING_FOR_LB:
      {
        return HANDLER_FUNC (UNP_WAITING_FOR_LB) (unp, t);
      }

    case UNP_WAITING_FOR_IDENT:
      {
        return HANDLER_FUNC (UNP_WAITING_FOR_IDENT) (unp, t);
      }

    case UNP_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return HANDLER_FUNC (UNP_WAITING_FOR_COMMA_OR_RIGHT) (unp, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
HANDLER_FUNC (UNP_WAITING_FOR_TYPE) (type_parser *unp, type t)
{
  union_parser_assert_state (unp, UNP_WAITING_FOR_TYPE);

  switch (unb_accept_type (&unp->unp.builder, t))
    {
    case ERR_NOMEM:
      {
        return SPR_MALLOC_ERROR;
      }
    case SUCCESS:
      {
        unp->unp.state = UNP_WAITING_FOR_COMMA_OR_RIGHT;
        return SPR_CONTINUE;
      }
    default:
      {
        ASSERT (0);
        return SPR_MALLOC_ERROR;
      }
    }

  return SPR_CONTINUE;
}

stackp_result
unp_accept_type (type_parser *unp, type type)
{
  union_parser_assert (unp);
  ASSERT (unp->state == TB_UNION);

  switch (unp->unp.state)
    {
    case UNP_WAITING_FOR_TYPE:
      {
        return HANDLER_FUNC (UNP_WAITING_FOR_TYPE) (unp, type);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
