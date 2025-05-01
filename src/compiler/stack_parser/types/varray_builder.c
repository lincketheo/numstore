#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/type_builder.h"
#include "compiler/tokens.h"
#include "utils/bounds.h"

////////////////////////// DEV

DEFINE_DBG_ASSERT_I (type_builder, varray_builder, s)
{
  ASSERT (s);
  ASSERT (s->state == TB_VARRAY);
}

static inline void
varray_builder_assert_state (type_builder *tb, int vab_state)
{
  varray_builder_assert (tb);
  ASSERT ((int)tb->vab.state == vab_state);
}

#define TOK_HANDLER_FUNC(state) vab_handle_tok_##state
#define TYPE_HANDLER_FUNC(state) vab_handle_type_##state

////////////////////////// API

stackp_result
vab_create (type_builder *dest)
{
  ASSERT (dest);

  dest->vab.rank = 1;
  dest->vab.state = VAB_WAITING_FOR_LEFT_OR_TYPE;
  dest->state = TB_VARRAY;

  varray_builder_assert (dest);

  return SPR_CONTINUE;
}

stackp_result
vab_build (type_builder *tb, lalloc *alloc)
{
  varray_builder_assert_state (tb, VAB_DONE);

  tb->ret.va.t = lmalloc (alloc, sizeof tb->ret.va.t);
  if (tb->ret.va.t == NULL)
    {
      return SPR_MALLOC_ERROR;
    }

  *tb->ret.va.t = tb->vab.type;
  tb->ret.va.rank = tb->vab.rank;

  return SPR_DONE;
}

sb_feed_t
vab_expect_next (const type_builder *tb, token t)
{
  varray_builder_assert (tb);

  switch (tb->vab.state)
    {
    case VAB_WAITING_FOR_LEFT_OR_TYPE:
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
TOK_HANDLER_FUNC (VAB_WAITING_FOR_RIGHT) (type_builder *sb, token t)
{
  varray_builder_assert_state (sb, VAB_WAITING_FOR_RIGHT);

  if (t.type != TT_RIGHT_BRACKET)
    {
      return SPR_SYNTAX_ERROR;
    }

  sb->vab.rank++;
  sb->vab.state = VAB_WAITING_FOR_LEFT_OR_TYPE;

  return SPR_CONTINUE;
}

static inline stackp_result
TOK_HANDLER_FUNC (VAB_WAITING_FOR_LEFT_OR_TYPE) (type_builder *sb, token t)
{
  varray_builder_assert_state (sb, VAB_WAITING_FOR_LEFT_OR_TYPE);

  if (t.type != TT_LEFT_BRACKET)
    {
      return SPR_SYNTAX_ERROR;
    }

  sb->vab.state = VAB_WAITING_FOR_LEFT_OR_TYPE;

  return SPR_CONTINUE;
}

stackp_result
vab_accept_token (type_builder *vab, token t)
{
  varray_builder_assert (vab);
  ASSERT (vab->state == TB_ENUM);

  switch (vab->vab.state)
    {
    case VAB_WAITING_FOR_RIGHT:
      {
        return TOK_HANDLER_FUNC (VAB_WAITING_FOR_RIGHT) (vab, t);
      }

    case VAB_WAITING_FOR_LEFT_OR_TYPE:
      {
        return TOK_HANDLER_FUNC (VAB_WAITING_FOR_LEFT_OR_TYPE) (vab, t);
      }

    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

////////////////////////// ACCEPT TYPE

static inline stackp_result
TYPE_HANDLER_FUNC (VAB_WAITING_FOR_LEFT_OR_TYPE) (type_builder *vab, type t)
{
  varray_builder_assert_state (vab, VAB_WAITING_FOR_LEFT_OR_TYPE);
  vab->vab.type = t;
  vab->vab.state = VAB_DONE;
  return SPR_DONE;
}

stackp_result
vab_accept_type (type_builder *vab, type t)
{
  varray_builder_assert (vab);
  ASSERT (vab->state == TB_ENUM);

  switch (vab->vab.state)
    {
    case VAB_WAITING_FOR_LEFT_OR_TYPE:
      {
        return TYPE_HANDLER_FUNC (VAB_WAITING_FOR_LEFT_OR_TYPE) (vab, t);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
