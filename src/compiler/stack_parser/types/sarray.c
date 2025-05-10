#include "type/builders/sarray.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/type_parser.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
#include "type/types.h"
#include "utils/bounds.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (type_parser, sarray_parser, s)
{
  ASSERT (s);
  ASSERT (s->state == TB_SARRAY);
}

static inline void
sarray_parser_assert_state (type_parser *tb, int sap_state)
{
  sarray_parser_assert (tb);
  ASSERT ((int)tb->sap.state == sap_state);
}

#define TOK_HANDLER_FUNC(state) sap_handle_tok_##state
#define TYPE_HANDLER_FUNC(state) sap_handle_type_##state

////////////////////////// API

stackp_result
sap_create (type_parser *dest, lalloc *alloc)
{
  ASSERT (dest);

  error e = error_create (NULL);
  switch (sab_create (&dest->sap.builder, alloc, &e))
    {
    case ERR_NOMEM:
      {
        return SPR_NOMEM;
      }
    case SUCCESS:
      {
        dest->sap.state = SAP_WAITING_FOR_NUMBER;
        dest->state = TB_SARRAY;

        sarray_parser_assert_state (
            dest, SAP_WAITING_FOR_NUMBER);

        return SPR_CONTINUE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
sap_build (type_parser *tb)
{
  sarray_parser_assert_state (tb, SAP_DONE);

  error e = error_create (NULL);
  switch (sab_build (&tb->ret.sa, &tb->sap.builder, &e))
    {
    case ERR_INVALID_ARGUMENT:
      {
        return SPR_SYNTAX_ERROR;
      }
    case SUCCESS:
      {
        tb->ret.type = T_SARRAY;
        return SPR_DONE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

sb_feed_t
sap_expect_next (const type_parser *tb, token t)
{
  sarray_parser_assert (tb);

  switch (tb->sap.state)
    {
    case SAP_WAITING_FOR_LEFT_OR_TYPE:
      {
        switch (t.type)
          {
          case TT_LEFT_BRACKET:
            {
              return SBFT_TOKEN;
            }
          default:
            {
              return SBFT_TYPE;
            }
          }
        break;
      }
    default:
      {
        return SBFT_TOKEN;
      }
    }
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
TOK_HANDLER_FUNC (SAP_WAITING_FOR_NUMBER) (
    type_parser *sb, token t)
{
  sarray_parser_assert_state (sb, SAP_WAITING_FOR_NUMBER);

  if (t.type != TT_INTEGER)
    {
      return SPR_SYNTAX_ERROR;
    }

  if (t.integer < 0)
    {
      return SPR_SYNTAX_ERROR;
    }

  error e = error_create (NULL);
  switch (sab_accept_dim (&sb->sap.builder, (u32)t.integer, &e))
    {
    case ERR_NOMEM:
      {
        return SPR_NOMEM;
      }
    case SUCCESS:
      {
        sb->sap.state = SAP_WAITING_FOR_RIGHT;
        return SPR_CONTINUE;
      }
    case ERR_INVALID_ARGUMENT:
      {
        return SPR_SYNTAX_ERROR;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline stackp_result
TOK_HANDLER_FUNC (SAP_WAITING_FOR_RIGHT) (type_parser *sb, token t)
{
  sarray_parser_assert_state (sb, SAP_WAITING_FOR_RIGHT);

  if (t.type != TT_RIGHT_BRACKET)
    {
      return SPR_SYNTAX_ERROR;
    }

  sb->sap.state = SAP_WAITING_FOR_LEFT_OR_TYPE;

  return SPR_CONTINUE;
}

static inline stackp_result
TOK_HANDLER_FUNC (SAP_WAITING_FOR_LEFT_OR_TYPE) (type_parser *sb, token t)
{
  sarray_parser_assert_state (sb, SAP_WAITING_FOR_LEFT_OR_TYPE);

  if (t.type != TT_LEFT_BRACKET)
    {
      return SPR_SYNTAX_ERROR;
    }

  sb->sap.state = SAP_WAITING_FOR_NUMBER;

  return SPR_CONTINUE;
}

stackp_result
sap_accept_token (type_parser *sab, token t)
{
  sarray_parser_assert (sab);

  switch (sab->sap.state)
    {
    case SAP_WAITING_FOR_NUMBER:
      {
        return TOK_HANDLER_FUNC (SAP_WAITING_FOR_NUMBER) (sab, t);
      }

    case SAP_WAITING_FOR_RIGHT:
      {
        return TOK_HANDLER_FUNC (SAP_WAITING_FOR_RIGHT) (sab, t);
      }

    case SAP_WAITING_FOR_LEFT_OR_TYPE:
      {
        return TOK_HANDLER_FUNC (SAP_WAITING_FOR_LEFT_OR_TYPE) (sab, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
TYPE_HANDLER_FUNC (SAP_WAITING_FOR_LEFT_OR_TYPE) (type_parser *sab, type t)
{
  sarray_parser_assert_state (sab, SAP_WAITING_FOR_LEFT_OR_TYPE);

  error e = error_create (NULL);
  switch (sab_accept_type (&sab->sap.builder, t, &e))
    {
    case ERR_NOMEM:
      {
        return SPR_NOMEM;
      }
    case SUCCESS:
      {
        sab->sap.state = SAP_DONE;
        return SPR_DONE;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
sap_accept_type (type_parser *sab, type t)
{
  sarray_parser_assert (sab);

  switch (sab->sap.state)
    {
    case SAP_WAITING_FOR_LEFT_OR_TYPE:
      {
        return TYPE_HANDLER_FUNC (SAP_WAITING_FOR_LEFT_OR_TYPE) (sab, t);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
