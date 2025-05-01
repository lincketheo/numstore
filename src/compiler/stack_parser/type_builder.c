#include "compiler/stack_parser/type_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/types/sarray_builder.h"
#include "compiler/stack_parser/types/union_builder.h"
#include "compiler/stack_parser/types/varray_builder.h"
#include "compiler/tokens.h"

DEFINE_DBG_ASSERT_I (type_builder, type_builder, typeb)
{
  ASSERT (typeb);
}

type_builder
typeb_create (void)
{
  return (type_builder){
    .state = TB_UNKNOWN,
  };
}

stackp_result
typeb_accept_token (type_builder *tb, token t, lalloc *alloc)
{
  type_builder_assert (tb);

  switch (tb->state)
    {
    case TB_UNKNOWN:
      {
        switch (t.type)
          {
          case TT_STRUCT:
            {
              return sb_create (tb, alloc);
            }
          case TT_UNION:
            {
              return ub_create (tb, alloc);
            }
          case TT_ENUM:
            {
              return eb_create (tb, alloc);
            }
          case TT_LEFT_BRACKET:
            {
              tb->state = TB_ARRAY_UNKNOWN;
              return SPR_CONTINUE;
            }
          case TT_PRIM:
            {
              return prim_create (tb, t.prim);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    case TB_ARRAY_UNKNOWN:
      {
        switch (t.type)
          {
          case TT_RIGHT_BRACKET:
            {
              return vab_create (tb);
            }
          case TT_INTEGER:
            {
              return sab_create (tb, alloc, t.integer);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    case TB_STRUCT:
      {
        return sb_accept_token (tb, t, alloc);
      }
    case TB_UNION:
      {
        return ub_accept_token (tb, t, alloc);
      }
    case TB_ENUM:
      {
        return eb_accept_token (tb, t, alloc);
      }
    case TB_SARRAY:
      {
        ASSERT (0);
        return 0;
      }
    case TB_VARRAY:
      {
        ASSERT (0);
        return 0;
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
  ASSERT (0);
  return 0;
}

stackp_result
typeb_accept_type (type_builder *tb, type t)
{
  type_builder_assert (tb);
  switch (tb->state)
    {
    case TB_STRUCT:
      {
        return sb_accept_type (tb, t);
      }
    case TB_UNION:
      {
        return ub_accept_type (tb, t);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

stackp_result
typeb_build (type_builder *tb, lalloc *alloc)
{
  type_builder_assert (tb);

  switch (tb->state)
    {
    case TB_STRUCT:
      {
        return sb_build (tb, alloc);
      }
    case TB_UNION:
      {
        return ub_build (tb, alloc);
      }
    case TB_ENUM:
      {
        return eb_build (tb, alloc);
      }
    case TB_PRIM:
      {
        return SPR_DONE;
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

#define TYPE_ON(arg, _state) \
  arg.state == _state ? SBFT_TYPE : SBFT_TOKEN

sb_feed_t
typeb_expect_next (const type_builder *tb, token t)
{
  switch (tb->state)
    {
    case TB_UNKNOWN:
      {
        return SBFT_TOKEN;
      }
    case TB_ARRAY_UNKNOWN:
      {
        return SBFT_TOKEN;
      }
    case TB_STRUCT:
      {
        return TYPE_ON (tb->sb, SB_WAITING_FOR_TYPE);
      }
    case TB_UNION:
      {
        return TYPE_ON (tb->ub, UB_WAITING_FOR_TYPE);
      }
    case TB_ENUM:
      {
        return SBFT_TOKEN;
      }
    case TB_VARRAY:
      {
        return vab_expect_next (tb, t);
      }
    case TB_SARRAY:
      {
        return sab_expect_next (tb, t);
      }
    default:
      {
        ASSERT (0);
      }
    }
  return 0;
}
