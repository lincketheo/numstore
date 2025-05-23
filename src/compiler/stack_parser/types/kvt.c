#include "compiler/stack_parser/types/kvt.h"

#include "ast/type/builders/kvt.h" // kvt_builder
#include "ast/type/types.h"
#include "dev/assert.h" // DEFINE_DBG_ASSERT_I
#include "mm/lalloc.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (kvt_parser, kvt_parser, s)
{
  ASSERT (s);
}

static const char *TAG = "Key Value Parser";

static inline void
kvt_parser_assert_state (kvt_parser *tb, int kvp_state)
{
  kvt_parser_assert (tb);
  ASSERT ((int)tb->state == kvp_state);
}

#define HANDLER_FUNC(state) kvp_handle_##state

////////////////////////// API

kvt_parser
kvp_create (lalloc *working, lalloc *destination, type_t type)
{
  ASSERT (type == T_STRUCT || type == T_UNION);

  kvt_parser ret = {
    .state = KVTP_WAITING_FOR_LB,
    .working_start = lalloc_get_state (working),
    .builder = kvb_create (working),
    .destination = destination,
    .result = {
        .type = type == T_STRUCT ? KVT_STRUCT : KVT_UNION,
    },
  };

  kvt_parser_assert_state (&ret, KVTP_WAITING_FOR_LB);

  return ret;
}

static stackp_result
kvp_build (kvt_parser *sb, error *e)
{
  kvt_parser_assert_state (sb, KVTP_DONE);

  err_t ret;
  switch (sb->result.type)
    {
    case KVT_UNION:
      {
        ret = kvb_union_t_build (
            &sb->result.un_res,
            &sb->builder,
            sb->destination, e);
        break;
      }
    case KVT_STRUCT:
      {
        ret = kvb_struct_t_build (
            &sb->result.st_res,
            &sb->builder,
            sb->destination, e);
        break;
      }
    }

  lalloc_reset_to_state (sb->builder.alloc, sb->working_start);

  switch (ret)
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

  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (KVTP_WAITING_FOR_LB) (kvt_parser *sb, token t, error *e)
{
  kvt_parser_assert_state (sb, KVTP_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return (stackp_result)error_causef (
          e, (err_t)SPR_SYNTAX_ERROR,
          "%s: Expecting token type %s, got %s",
          TAG, tt_tostr (TT_LEFT_BRACE), tt_tostr (t.type));
    }

  sb->state = KVTP_WAITING_FOR_IDENT;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (KVTP_WAITING_FOR_IDENT) (kvt_parser *sb, token t, error *e)
{
  kvt_parser_assert_state (sb, KVTP_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  switch (kvb_accept_key (&sb->builder, t.str, e))
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
        sb->state = KVTP_WAITING_FOR_TYPE;
        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline stackp_result
HANDLER_FUNC (KVTP_WAITING_FOR_COMMA_OR_RIGHT) (kvt_parser *sb, token t, error *e)
{
  kvt_parser_assert_state (sb, KVTP_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      sb->state = KVTP_WAITING_FOR_IDENT;
      return SPR_CONTINUE;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      sb->state = KVTP_DONE;
      return kvp_build (sb, e);
    }

  return (stackp_result)error_causef (
      e, (err_t)SPR_SYNTAX_ERROR,
      "%s: Expecting token type %s or %s, got %s",
      TAG, tt_tostr (TT_COMMA),
      tt_tostr (TT_RIGHT_BRACE),
      tt_tostr (t.type));

  return SPR_SYNTAX_ERROR;
}

stackp_result
kvp_accept_token (kvt_parser *tb, token t, error *e)
{
  kvt_parser_assert (tb);

  switch (tb->state)
    {
    case KVTP_WAITING_FOR_LB:
      {
        return HANDLER_FUNC (KVTP_WAITING_FOR_LB) (tb, t, e);
      }

    case KVTP_WAITING_FOR_IDENT:
      {
        return HANDLER_FUNC (KVTP_WAITING_FOR_IDENT) (tb, t, e);
      }

    case KVTP_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return HANDLER_FUNC (KVTP_WAITING_FOR_COMMA_OR_RIGHT) (tb, t, e);
      }

    default:
      {
        UNREACHABLE ();
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
HANDLER_FUNC (KVTP_WAITING_FOR_TYPE) (kvt_parser *sb, type t, error *e)
{
  kvt_parser_assert_state (sb, KVTP_WAITING_FOR_TYPE);

  switch (kvb_accept_type (&sb->builder, t, e))
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
        sb->state = KVTP_WAITING_FOR_COMMA_OR_RIGHT;
        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
kvp_accept_type (kvt_parser *sb, type type, error *e)
{
  kvt_parser_assert (sb);

  switch (sb->state)
    {
    case KVTP_WAITING_FOR_TYPE:
      {
        return HANDLER_FUNC (KVTP_WAITING_FOR_TYPE) (sb, type, e);
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}
