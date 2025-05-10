#include "compiler/stack_parser/types/struct.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/type_parser.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
#include "type/builders/struct.h"
#include "type/types.h"
#include "utils/bounds.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (type_parser, struct_parser, s)
{
  ASSERT (s);
  ASSERT (s->state == TB_STRUCT);
}

static inline void
struct_parser_assert_state (type_parser *tb, int stp_state)
{
  struct_parser_assert (tb);
  ASSERT ((int)tb->stp.state == stp_state);
}

#define HANDLER_FUNC(state) stp_handle_##state

////////////////////////// API

stackp_result
stp_create (type_parser *dest, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->state == TB_UNKNOWN);

  switch (stb_create (&dest->stp.builder, alloc, NULL))
    {
    case ERR_NOMEM:
      {
        return SPR_NOMEM;
      }
    case SUCCESS:
      {
        dest->stp.state = STP_WAITING_FOR_LB;
        dest->state = TB_STRUCT;

        struct_parser_assert_state (
            dest, STP_WAITING_FOR_LB);

        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
stp_build (type_parser *sb)
{
  struct_parser_assert_state (sb, STP_DONE);

  switch (stb_build (&sb->ret.st, &sb->stp.builder, NULL))
    {
    case ERR_INVALID_ARGUMENT:
      {
        return SPR_SYNTAX_ERROR;
      }
    case SUCCESS:
      {
        sb->ret.type = T_STRUCT;
        return SPR_DONE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }

  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (STP_WAITING_FOR_LB) (type_parser *sb, token t)
{
  struct_parser_assert_state (sb, STP_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return SPR_SYNTAX_ERROR;
    }

  sb->stp.state = STP_WAITING_FOR_IDENT;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (STP_WAITING_FOR_IDENT) (
    type_parser *sb, token t)
{
  struct_parser_assert_state (sb, STP_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  switch (stb_accept_key (&sb->stp.builder, t.str, NULL))
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
        sb->stp.state = STP_WAITING_FOR_TYPE;
        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline stackp_result
HANDLER_FUNC (STP_WAITING_FOR_COMMA_OR_RIGHT) (type_parser *sb, token t)
{
  struct_parser_assert_state (
      sb, STP_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      sb->stp.state = STP_WAITING_FOR_IDENT;
      return SPR_CONTINUE;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      sb->stp.state = STP_DONE;
      return SPR_DONE;
    }

  return SPR_SYNTAX_ERROR;
}

stackp_result
stp_accept_token (type_parser *tb, token t)
{
  struct_parser_assert (tb);
  ASSERT (tb->state == TB_STRUCT);

  switch (tb->stp.state)
    {
    case STP_WAITING_FOR_LB:
      {
        return HANDLER_FUNC (STP_WAITING_FOR_LB) (tb, t);
      }

    case STP_WAITING_FOR_IDENT:
      {
        return HANDLER_FUNC (STP_WAITING_FOR_IDENT) (tb, t);
      }

    case STP_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return HANDLER_FUNC (STP_WAITING_FOR_COMMA_OR_RIGHT) (tb, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
HANDLER_FUNC (STP_WAITING_FOR_TYPE) (type_parser *sb, type t)
{
  struct_parser_assert_state (sb, STP_WAITING_FOR_TYPE);

  switch (stb_accept_type (&sb->stp.builder, t, NULL))
    {
    case ERR_NOMEM:
      {
        return SPR_NOMEM;
      }
    case SUCCESS:
      {
        sb->stp.state = STP_WAITING_FOR_COMMA_OR_RIGHT;
        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
stp_accept_type (type_parser *sb, type type)
{
  struct_parser_assert (sb);

  switch (sb->stp.state)
    {
    case STP_WAITING_FOR_TYPE:
      {
        return HANDLER_FUNC (STP_WAITING_FOR_TYPE) (sb, type);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
