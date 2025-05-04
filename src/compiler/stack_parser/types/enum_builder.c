#include "compiler/stack_parser/types/enum_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/type_builder.h"
#include "compiler/tokens.h"
#include "typing.h"
#include "utils/bounds.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (type_builder, enum_builder, s)
{
  ASSERT (s);
  ASSERT (s->state == TB_ENUM);
  ASSERT (s->eb.keys);
  ASSERT (s->eb.cap > 0);
  ASSERT (s->eb.len <= s->eb.cap);
}

static inline void
enum_builder_assert_state (type_builder *tb, int eb_state)
{
  enum_builder_assert (tb);
  ASSERT ((int)tb->eb.state == eb_state);
}

#define HANDLER_FUNC(state) eb_handle_##state

////////////////////////// API

stackp_result
eb_create (type_builder *dest, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->state == TB_UNKNOWN);

  dest->eb.keys = lmalloc (alloc, 5 * sizeof *dest->eb.keys);
  if (!dest->eb.keys)
    {
      return SPR_MALLOC_ERROR;
    }

  dest->eb.cap = 5;
  dest->eb.len = 0;
  dest->eb.state = EB_WAITING_FOR_LB;
  dest->state = TB_ENUM;

  enum_builder_assert (dest);

  return SPR_CONTINUE;
}

stackp_result
eb_build (type_builder *eb, lalloc *alloc)
{
  enum_builder_assert_state (eb, EB_DONE);

  string *strs = eb->eb.keys;

  // Clip buffers
  if (eb->eb.len < eb->eb.cap)
    {
      strs = lrealloc (alloc, strs, eb->eb.len * sizeof *strs);
      if (!strs)
        {
          return SPR_MALLOC_ERROR;
        }
    }

  eb->ret.en.keys = strs;
  eb->ret.en.len = eb->eb.len;
  eb->ret.type = T_ENUM;

  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (EB_WAITING_FOR_LB) (type_builder *eb, token t)
{
  enum_builder_assert_state (eb, EB_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return SPR_SYNTAX_ERROR;
    }

  eb->eb.state = EB_WAITING_FOR_IDENT;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (EB_WAITING_FOR_IDENT) (
    type_builder *eb, token t, lalloc *alloc)
{
  enum_builder_assert_state (eb, EB_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  // Check for size adjustments
  if (eb->eb.len == eb->eb.cap)
    {
      // Restrict memory requirements to avoid overflow
      u32 ncap = 2 * eb->eb.cap;

      string *keys = lrealloc (alloc, eb->eb.keys, ncap * sizeof (string));
      if (!keys)
        {
          return SPR_MALLOC_ERROR;
        }

      eb->eb.keys = keys;
      eb->eb.cap = ncap;
    }

  eb->eb.keys[eb->eb.len++] = t.str;
  eb->eb.state = EB_WAITING_FOR_COMMA_OR_RIGHT;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (EB_WAITING_FOR_COMMA_OR_RIGHT) (type_builder *eb, token t)
{
  enum_builder_assert_state (eb, EB_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      eb->eb.state = EB_WAITING_FOR_IDENT;
      return SPR_CONTINUE;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      eb->eb.state = EB_DONE;
      return SPR_DONE;
    }

  return SPR_SYNTAX_ERROR;
}

stackp_result
eb_accept_token (type_builder *eb, token t, lalloc *alloc)
{
  enum_builder_assert (eb);
  ASSERT (eb->state == TB_ENUM);

  switch (eb->eb.state)
    {
    case EB_WAITING_FOR_LB:
      {
        return HANDLER_FUNC (EB_WAITING_FOR_LB) (eb, t);
      }

    case EB_WAITING_FOR_IDENT:
      {
        return HANDLER_FUNC (EB_WAITING_FOR_IDENT) (eb, t, alloc);
      }

    case EB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return HANDLER_FUNC (EB_WAITING_FOR_COMMA_OR_RIGHT) (eb, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
