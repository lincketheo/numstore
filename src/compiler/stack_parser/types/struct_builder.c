#include "compiler/stack_parser/types/struct_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/type_builder.h"
#include "compiler/tokens.h"
#include "utils/bounds.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (type_builder, struct_builder, s)
{
  ASSERT (s);
  ASSERT (s->state == TB_STRUCT);
  ASSERT (s->sb.keys);
  ASSERT (s->sb.keys);
  ASSERT (s->sb.cap > 0);
  ASSERT (s->sb.len <= s->sb.cap);
}

static inline void
struct_builder_assert_state (type_builder *tb, int sb_state)
{
  struct_builder_assert (tb);
  ASSERT ((int)tb->sb.state == sb_state);
}

#define HANDLER_FUNC(state) sb_handle_##state

////////////////////////// API

stackp_result
sb_create (type_builder *dest, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->state == TB_UNKNOWN);

  dest->sb.keys = lmalloc (alloc, 5 * sizeof *dest->sb.keys);
  if (!dest->sb.keys)
    {
      return SPR_MALLOC_ERROR;
    }
  dest->sb.types = lmalloc (alloc, 5 * sizeof *dest->sb.types);
  if (!dest->sb.types)
    {
      lfree (alloc, dest->sb.keys);
      return SPR_MALLOC_ERROR;
    }

  dest->sb.cap = 5;
  dest->sb.len = 0;
  dest->sb.state = SB_WAITING_FOR_LB;
  dest->state = TB_STRUCT;

  struct_builder_assert_state (dest, SB_WAITING_FOR_LB);

  return SPR_CONTINUE;
}

stackp_result
sb_build (type_builder *sb, lalloc *alloc)
{
  struct_builder_assert_state (sb, SB_DONE);

  type *types = sb->sb.types;
  string *strs = sb->sb.keys;

  // Clip buffers
  if (sb->sb.len < sb->sb.cap)
    {
      types = lrealloc (alloc, types, sb->sb.len * sizeof *types);
      if (!types)
        {
          return SPR_MALLOC_ERROR;
        }

      strs = lrealloc (alloc, strs, sb->sb.len * sizeof *strs);
      if (!strs)
        {
          return SPR_MALLOC_ERROR;
        }
      sb->sb.cap = sb->sb.len;
    }

  sb->ret.st.types = types;
  sb->ret.st.keys = strs;
  sb->ret.st.len = sb->sb.len;
  sb->ret.type = T_STRUCT;

  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (SB_WAITING_FOR_LB) (type_builder *sb, token t)
{
  struct_builder_assert_state (sb, SB_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return SPR_SYNTAX_ERROR;
    }

  sb->sb.state = SB_WAITING_FOR_IDENT;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (SB_WAITING_FOR_IDENT) (
    type_builder *sb, token t, lalloc *alloc)
{
  struct_builder_assert_state (sb, SB_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  // Check for size adjustments
  if (sb->sb.len == sb->sb.cap)
    {
      ASSERT (can_mul_u64 (sb->sb.cap, 2));
      ASSERT (can_mul_u64 (2 * sb->sb.cap, sizeof (string)));

      u32 ncap = 2 * sb->sb.cap;
      string *keys = lrealloc (alloc, sb->sb.keys, ncap * sizeof *keys);
      if (!keys)
        {
          return SPR_MALLOC_ERROR;
        }
      type *types = lrealloc (alloc, sb->sb.types, ncap * sizeof *types);
      if (!keys)
        {
          lfree (alloc, keys);
          return SPR_MALLOC_ERROR;
        }
      sb->sb.keys = keys;
      sb->sb.types = types;
      sb->sb.cap = ncap;
    }

  sb->sb.keys[sb->sb.len] = t.str; // DONT INC LEN - WAIT FOR TYPE ADD
  sb->sb.state = SB_WAITING_FOR_TYPE;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (SB_WAITING_FOR_COMMA_OR_RIGHT) (type_builder *sb, token t)
{
  struct_builder_assert_state (sb, SB_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      sb->sb.state = SB_WAITING_FOR_IDENT;
      return SPR_CONTINUE;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      sb->sb.state = SB_DONE;
      return SPR_DONE;
    }

  return SPR_SYNTAX_ERROR;
}

stackp_result
sb_accept_token (type_builder *tb, token t, lalloc *alloc)
{
  struct_builder_assert (tb);
  ASSERT (tb->state == TB_STRUCT);

  switch (tb->sb.state)
    {
    case SB_WAITING_FOR_LB:
      {
        return HANDLER_FUNC (SB_WAITING_FOR_LB) (tb, t);
      }

    case SB_WAITING_FOR_IDENT:
      {
        return HANDLER_FUNC (SB_WAITING_FOR_IDENT) (tb, t, alloc);
      }

    case SB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return HANDLER_FUNC (SB_WAITING_FOR_COMMA_OR_RIGHT) (tb, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
HANDLER_FUNC (SB_WAITING_FOR_TYPE) (type_builder *sb, type t)
{
  struct_builder_assert_state (sb, SB_WAITING_FOR_TYPE);
  ASSERT (sb->sb.len < sb->sb.cap); // From previous string push

  sb->sb.types[sb->sb.len++] = t; // NOW YOU CAN INC LEN
  sb->sb.state = SB_WAITING_FOR_COMMA_OR_RIGHT;

  return SPR_CONTINUE;
}

stackp_result
sb_accept_type (type_builder *sb, type type)
{
  struct_builder_assert (sb);

  switch (sb->sb.state)
    {
    case SB_WAITING_FOR_TYPE:
      {
        return HANDLER_FUNC (SB_WAITING_FOR_TYPE) (sb, type);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
