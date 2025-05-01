#include "compiler/stack_parser/types/union_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/type_builder.h"
#include "compiler/tokens.h"
#include "utils/bounds.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (type_builder, union_builder, s)
{
  ASSERT (s);
  ASSERT (s->state == TB_UNION);
  ASSERT (s->ub.keys);
  ASSERT (s->ub.keys);
  ASSERT (s->ub.cap > 0);
  ASSERT (s->ub.len <= s->ub.cap);
}

static inline void
union_builder_assert_state (type_builder *ub, int ub_state)
{
  union_builder_assert (ub);
  ASSERT ((int)ub->ub.state == ub_state);
}

#define HANDLER_FUNC(state) ub_handle_##state

////////////////////////// API

stackp_result
ub_create (type_builder *dest, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->state == TB_UNKNOWN);

  dest->ub.keys = lmalloc (alloc, 5 * sizeof *dest->ub.keys);
  if (!dest->ub.keys)
    {
      return SPR_MALLOC_ERROR;
    }

  dest->ub.types = lmalloc (alloc, 5 * sizeof *dest->ub.types);
  if (!dest->ub.types)
    {
      lfree (alloc, dest->ub.keys);
      return SPR_MALLOC_ERROR;
    }

  dest->ub.cap = 5;
  dest->ub.len = 0;
  dest->ub.state = UB_WAITING_FOR_LB;
  dest->state = TB_UNION;

  union_builder_assert_state (dest, UB_WAITING_FOR_LB);

  return SPR_CONTINUE;
}

stackp_result
ub_build (type_builder *ub, lalloc *alloc)
{
  union_builder_assert (ub);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_COMMA_OR_RIGHT);

  type *types = ub->ub.types;
  string *strs = ub->ub.keys;

  // Clip buffers
  if (ub->ub.len < ub->ub.cap)
    {
      types = lrealloc (alloc, types, ub->ub.len * sizeof *types);
      if (!types)
        {
          return SPR_MALLOC_ERROR;
        }

      strs = lrealloc (alloc, strs, ub->ub.len * sizeof *strs);
      if (!strs)
        {
          return SPR_MALLOC_ERROR;
        }
    }

  ub->ret.un.types = types;
  ub->ret.un.keys = strs;
  ub->ret.un.len = ub->ub.len;
  ub->ret.type = T_UNION;

  return SPR_DONE;
}

////////////////////////// ACCEPT TOKEN

static inline stackp_result
HANDLER_FUNC (UB_WAITING_FOR_LB) (type_builder *ub, token t)
{
  union_builder_assert_state (ub, UB_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return SPR_SYNTAX_ERROR;
    }

  ub->ub.state = UB_WAITING_FOR_IDENT;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (UB_WAITING_FOR_IDENT) (
    type_builder *ub, token t, lalloc *alloc)
{
  union_builder_assert_state (ub, UB_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return SPR_SYNTAX_ERROR;
    }

  // Check for size adjustments
  if (ub->ub.len == ub->ub.cap)
    {
      // Restrict memory requirements to avoid overflow
      ASSERT (can_mul_u64 (ub->ub.cap, 2));
      ASSERT (can_mul_u64 (2 * ub->ub.cap, sizeof (string)));
      u32 ncap = 2 * ub->ub.cap;
      string *keys = lrealloc (alloc, ub->ub.keys, ncap * sizeof (string));
      if (!keys)
        {
          return SPR_MALLOC_ERROR;
        }
      type *types = lrealloc (alloc, ub->ub.types, ncap * sizeof (type));
      if (!keys)
        {
          lfree (alloc, keys);
          return SPR_MALLOC_ERROR;
        }
      ub->ub.keys = keys;
      ub->ub.types = types;
      ub->ub.cap = ncap;
    }

  ub->ub.keys[ub->ub.len] = t.str; // DONT INC LEN - WAIT FOR TYPE ADD
  ub->ub.state = UB_WAITING_FOR_TYPE;

  return SPR_CONTINUE;
}

static inline stackp_result
HANDLER_FUNC (UB_WAITING_FOR_COMMA_OR_RIGHT) (type_builder *ub, token t)
{
  union_builder_assert_state (ub, UB_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      ub->ub.state = UB_WAITING_FOR_IDENT;
      return SPR_CONTINUE;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      ub->ub.state = UB_DONE;
      return SPR_DONE;
    }

  return SPR_SYNTAX_ERROR;
}

stackp_result
ub_accept_token (type_builder *ub, token t, lalloc *alloc)
{
  union_builder_assert (ub);
  ASSERT (ub->state == TB_UNION);

  switch (ub->ub.state)
    {

    case UB_WAITING_FOR_LB:
      {
        return HANDLER_FUNC (UB_WAITING_FOR_LB) (ub, t);
      }

    case UB_WAITING_FOR_IDENT:
      {
        return HANDLER_FUNC (UB_WAITING_FOR_IDENT) (ub, t, alloc);
      }

    case UB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return HANDLER_FUNC (UB_WAITING_FOR_COMMA_OR_RIGHT) (ub, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
HANDLER_FUNC (UB_WAITING_FOR_TYPE) (type_builder *ub, type t)
{
  union_builder_assert_state (ub, UB_WAITING_FOR_TYPE);
  ASSERT (ub->ub.len < ub->ub.cap);

  ub->ub.types[ub->ub.len++] = t; // NOW YOU CAN INC LEN
  ub->ub.state = UB_WAITING_FOR_COMMA_OR_RIGHT;

  return SPR_CONTINUE;
}

stackp_result
ub_accept_type (type_builder *ub, type type)
{
  union_builder_assert (ub);
  ASSERT (ub->state == TB_UNION);

  switch (ub->ub.state)
    {
    case UB_WAITING_FOR_TYPE:
      {
        return HANDLER_FUNC (UB_WAITING_FOR_TYPE) (ub, type);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
