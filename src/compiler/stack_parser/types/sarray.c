#include "compiler/stack_parser/types/sarray.h"

#include "ast/type/builders/sarray.h"
#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
#include "errors/error.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (sarray_parser, sarray_parser, s)
{
  ASSERT (s);
}

static const char *TAG = "Strict Array Parser ";

static inline void
sarray_parser_assert_state (sarray_parser *tb, int sap_state)
{
  sarray_parser_assert (tb);
  ASSERT ((int)tb->state == sap_state);
}

#define TOK_HANDLER_FUNC(state) sap_handle_tok_##state
#define TYPE_HANDLER_FUNC(state) sap_handle_type_##state

////////////////////////// API

sarray_parser
sap_create (lalloc *alloc)
{
  ASSERT (alloc);

  sarray_parser ret = {
    .state = SAP_WAITING_FOR_NUMBER,
    .builder = sab_create (alloc),
  };
  sarray_parser_assert_state (&ret, SAP_WAITING_FOR_NUMBER);
  return ret;
}

stackp_result
sap_build (sarray_t *dest, sarray_parser *tb, lalloc *destination, error *e)
{
  sarray_parser_assert_state (tb, SAP_DONE);

  switch (sab_build (dest, &tb->builder, destination, e))
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
TOK_HANDLER_FUNC (SAP_WAITING_FOR_NUMBER) (sarray_parser *sb, token t, error *e)
{
  sarray_parser_assert_state (sb, SAP_WAITING_FOR_NUMBER);

  if (t.type != TT_INTEGER)
    {
      return (stackp_result)error_causef (
          e, (err_t)SPR_SYNTAX_ERROR,
          "%s: Expecting token type: %s, got: %s",
          TAG, tt_tostr (TT_INTEGER), tt_tostr (t.type));
    }

  if (t.integer < 0)
    {
      return (stackp_result)error_causef (
          e, (err_t)SPR_SYNTAX_ERROR,
          "%s: Expecting dim > 0. Got dim: %d",
          TAG, t.integer);
    }

  switch (sab_accept_dim (&sb->builder, (u32)t.integer, e))
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
        sb->state = SAP_WAITING_FOR_RIGHT;
        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline stackp_result
TOK_HANDLER_FUNC (SAP_WAITING_FOR_RIGHT) (sarray_parser *sb, token t, error *e)
{
  sarray_parser_assert_state (sb, SAP_WAITING_FOR_RIGHT);

  if (t.type != TT_RIGHT_BRACKET)
    {
      return (stackp_result)error_causef (
          e, (err_t)SPR_SYNTAX_ERROR,
          "%s: Expecting token type: %s, got: %s",
          TAG, tt_tostr (TT_RIGHT_BRACKET), tt_tostr (t.type));
    }

  sb->state = SAP_WAITING_FOR_LEFT_OR_TYPE;

  return SPR_CONTINUE;
}

static inline stackp_result
TOK_HANDLER_FUNC (SAP_WAITING_FOR_LEFT_OR_TYPE) (sarray_parser *sb, token t, error *e)
{
  sarray_parser_assert_state (sb, SAP_WAITING_FOR_LEFT_OR_TYPE);

  if (t.type != TT_LEFT_BRACKET)
    {
      return (stackp_result)error_causef (
          e, (err_t)TT_LEFT_BRACKET,
          "%s: Expecting token type: %s, got: %s",
          TAG, tt_tostr (TT_RIGHT_BRACKET), tt_tostr (t.type));
    }

  sb->state = SAP_WAITING_FOR_NUMBER;

  return SPR_CONTINUE;
}

stackp_result
sap_accept_token (sarray_parser *sab, token t, error *e)
{
  sarray_parser_assert (sab);

  switch (sab->state)
    {
    case SAP_WAITING_FOR_NUMBER:
      {
        return TOK_HANDLER_FUNC (SAP_WAITING_FOR_NUMBER) (sab, t, e);
      }

    case SAP_WAITING_FOR_RIGHT:
      {
        return TOK_HANDLER_FUNC (SAP_WAITING_FOR_RIGHT) (sab, t, e);
      }

    case SAP_WAITING_FOR_LEFT_OR_TYPE:
      {
        return TOK_HANDLER_FUNC (SAP_WAITING_FOR_LEFT_OR_TYPE) (sab, t, e);
      }

    default:
      {
        UNREACHABLE ();
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
TYPE_HANDLER_FUNC (SAP_WAITING_FOR_LEFT_OR_TYPE) (sarray_parser *sab, type t, error *e)
{
  sarray_parser_assert_state (sab, SAP_WAITING_FOR_LEFT_OR_TYPE);

  switch (sab_accept_type (&sab->builder, t, e))
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
        sab->state = SAP_DONE;
        return SPR_DONE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
sap_accept_type (sarray_parser *sab, type t, error *e)
{
  sarray_parser_assert (sab);

  switch (sab->state)
    {
    case SAP_WAITING_FOR_LEFT_OR_TYPE:
      {
        return TYPE_HANDLER_FUNC (SAP_WAITING_FOR_LEFT_OR_TYPE) (sab, t, e);
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}
