#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/type_builder.h"
#include "compiler/tokens.h"
#include "typing.h"
#include "utils/bounds.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (type_builder, sarray_builder, s)
{
  ASSERT (s);
  ASSERT (s->state == TB_SARRAY);
  ASSERT (s->sab.dims);
  ASSERT (s->sab.len > 0);
  ASSERT (s->sab.cap > 0);
  ASSERT (s->sab.len <= s->sab.len);

  for (u32 i = 0; i < s->sab.len; ++i)
    {
      ASSERT (s->sab.dims[i] > 0);
    }
}

static inline void
sarray_builder_assert_state (type_builder *tb, int sab_state)
{
  sarray_builder_assert (tb);
  ASSERT ((int)tb->sab.state == sab_state);
}

#define TOK_HANDLER_FUNC(state) sab_handle_tok_##state
#define TYPE_HANDLER_FUNC(state) sab_handle_type_##state

////////////////////////// API

stackp_result
sab_create (type_builder *dest, lalloc *alloc, u32 dim)
{
  ASSERT (dest);

  dest->sab.dims = lmalloc (alloc, 5 * sizeof *dest->sab.dims);
  if (!dest->sab.dims)
    {
      return SPR_MALLOC_ERROR;
    }
  dest->sab.cap = 5;
  dest->sab.len = 0;
  dest->sab.dims[dest->sab.len++] = dim;
  dest->sab.state = SAB_WAITING_FOR_RIGHT;
  dest->state = TB_SARRAY;

  sarray_builder_assert (dest);

  return SPR_CONTINUE;
}

stackp_result
sab_build (type_builder *tb, lalloc *alloc)
{
  sarray_builder_assert_state (tb, SAB_DONE);

  tb->ret.sa.t = lmalloc (alloc, sizeof *tb->ret.sa.t);
  if (tb->ret.sa.t == NULL)
    {
      return SPR_MALLOC_ERROR;
    }

  u32 *dims = tb->sab.dims;

  // Clip buffers
  if (tb->sab.len < tb->sab.cap)
    {
      dims = lrealloc (alloc, dims, tb->sab.len * sizeof *dims);
      if (!dims)
        {
          return SPR_MALLOC_ERROR;
        }
      tb->sab.cap = tb->sab.len;
    }

  tb->ret.type = T_SARRAY;
  *tb->ret.sa.t = tb->sab.type;
  tb->ret.sa.dims = dims;
  tb->ret.sa.rank = tb->sab.len;

  return SPR_DONE;
}

sb_feed_t
sab_expect_next (const type_builder *tb, token t)
{
  sarray_builder_assert (tb);

  switch (tb->sab.state)
    {
    case SAB_WAITING_FOR_LEFT_OR_TYPE:
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
TOK_HANDLER_FUNC (SAB_WAITING_FOR_NUMBER) (
    type_builder *sb, token t, lalloc *alloc)
{
  sarray_builder_assert_state (sb, SAB_WAITING_FOR_NUMBER);

  if (t.type != TT_INTEGER)
    {
      return SPR_SYNTAX_ERROR;
    }
  if (t.integer <= 0)
    {
      return SPR_SYNTAX_ERROR;
    }

  // TODO - maximum dimension size?

  // Check for size adjustments
  if (sb->sab.len == sb->sab.cap)
    {
      // Restrict memory requirements to avoid overflow
      ASSERT (can_mul_u64 (sb->sab.cap, 2));
      ASSERT (can_mul_u64 (2 * sb->sab.cap, sizeof (string)));
      u32 ncap = 2 * sb->sab.cap;

      u32 *dims = lrealloc (alloc, sb->sab.dims, ncap * sizeof (string));
      if (!dims)
        {
          return SPR_MALLOC_ERROR;
        }

      sb->sab.dims = dims;
      sb->sab.cap = ncap;
    }

  sb->sab.dims[sb->sab.len++] = (u32)t.integer;
  sb->sab.state = SAB_WAITING_FOR_RIGHT;

  return SPR_CONTINUE;
}

static inline stackp_result
TOK_HANDLER_FUNC (SAB_WAITING_FOR_RIGHT) (type_builder *sb, token t)
{
  sarray_builder_assert_state (sb, SAB_WAITING_FOR_RIGHT);

  if (t.type != TT_RIGHT_BRACKET)
    {
      return SPR_SYNTAX_ERROR;
    }

  sb->sab.state = SAB_WAITING_FOR_LEFT_OR_TYPE;

  return SPR_CONTINUE;
}

static inline stackp_result
TOK_HANDLER_FUNC (SAB_WAITING_FOR_LEFT_OR_TYPE) (type_builder *sb, token t)
{
  sarray_builder_assert_state (sb, SAB_WAITING_FOR_LEFT_OR_TYPE);

  if (t.type != TT_LEFT_BRACKET)
    {
      return SPR_SYNTAX_ERROR;
    }

  sb->sab.state = SAB_WAITING_FOR_NUMBER;

  return SPR_CONTINUE;
}

stackp_result
sab_accept_token (type_builder *sab, token t, lalloc *alloc)
{
  sarray_builder_assert (sab);

  switch (sab->sab.state)
    {
    case SAB_WAITING_FOR_NUMBER:
      {
        return TOK_HANDLER_FUNC (SAB_WAITING_FOR_NUMBER) (sab, t, alloc);
      }

    case SAB_WAITING_FOR_RIGHT:
      {
        return TOK_HANDLER_FUNC (SAB_WAITING_FOR_RIGHT) (sab, t);
      }

    case SAB_WAITING_FOR_LEFT_OR_TYPE:
      {
        return TOK_HANDLER_FUNC (SAB_WAITING_FOR_LEFT_OR_TYPE) (sab, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
TYPE_HANDLER_FUNC (SAB_WAITING_FOR_LEFT_OR_TYPE) (type_builder *sab, type t)
{
  sarray_builder_assert_state (sab, SAB_WAITING_FOR_LEFT_OR_TYPE);
  sab->sab.type = t;
  sab->sab.state = SAB_DONE;
  return SPR_DONE;
}

stackp_result
sab_accept_type (type_builder *sab, type t)
{
  sarray_builder_assert (sab);

  switch (sab->sab.state)
    {
    case SAB_WAITING_FOR_LEFT_OR_TYPE:
      {
        return TYPE_HANDLER_FUNC (SAB_WAITING_FOR_LEFT_OR_TYPE) (sab, t);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
