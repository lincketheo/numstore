#include "compiler/ast_builders/type_builder.h"
#include "compiler/ast_builders/common.h"
#include "compiler/ast_builders/types/sarray_builder.h"
#include "compiler/ast_builders/types/union_builder.h"
#include "compiler/ast_builders/types/varray_builder.h"
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

parse_result
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
              return PR_CONTINUE;
            }
          case TT_PRIM:
            {
              return prim_create (tb, t.prim);
            }
          default:
            {
              return PR_SYNTAX_ERROR;
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
              return PR_SYNTAX_ERROR;
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
        return PR_SYNTAX_ERROR;
      }
    }
  ASSERT (0);
  return 0;
}

parse_result
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
        return PR_SYNTAX_ERROR;
      }
    }
}

parse_result
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
        return PR_DONE;
      }
    default:
      {
        return PR_SYNTAX_ERROR;
      }
    }
}

parse_expect
typeb_expect_next (const type_builder *tb, token t)
{
  switch (tb->state)
    {
    case TB_UNKNOWN:
      {
        return PE_TOKEN;
      }
    case TB_ARRAY_UNKNOWN:
      {
        return PE_TOKEN;
      }
    case TB_STRUCT:
      {
        return PE_TOKEN;
      }
    case TB_UNION:
      {
        return ub_expect_next (tb);
      }
    case TB_ENUM:
      {
        return PE_TOKEN;
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
