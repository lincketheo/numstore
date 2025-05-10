#include "compiler/stack_parser/type_parser.h"
#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"

DEFINE_DBG_ASSERT_I (type_parser, type_parser, typep)
{
  ASSERT (typep);
}

type_parser
typep_create (lalloc *alloc)
{
  return (type_parser){
    .state = TB_UNKNOWN,
    .alloc = alloc,
  };
}

stackp_result
typep_accept_token (type_parser *tb, token t)
{
  type_parser_assert (tb);

  switch (tb->state)
    {
    case TB_UNKNOWN:
      {
        switch (t.type)
          {
          case TT_STRUCT:
            {
              return stp_create (tb, tb->alloc);
            }
          case TT_UNION:
            {
              return unp_create (tb, tb->alloc);
            }
          case TT_ENUM:
            {
              return enp_create (tb, tb->alloc);
            }
          case TT_LEFT_BRACKET:
            {
              return sap_create (tb, tb->alloc);
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
    case TB_STRUCT:
      {
        return stp_accept_token (tb, t);
      }
    case TB_UNION:
      {
        return unp_accept_token (tb, t);
      }
    case TB_ENUM:
      {
        return enp_accept_token (tb, t);
      }
    case TB_SARRAY:
      {
        return sap_accept_token (tb, t);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
  UNREACHABLE ();
  return 0;
}

stackp_result
typep_accept_type (type_parser *tb, type t)
{
  type_parser_assert (tb);
  switch (tb->state)
    {
    case TB_STRUCT:
      {
        return stp_accept_type (tb, t);
      }
    case TB_UNION:
      {
        return unp_accept_type (tb, t);
      }
    case TB_SARRAY:
      {
        return sap_accept_type (tb, t);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}

stackp_result
typep_build (type_parser *tb)
{
  type_parser_assert (tb);

  switch (tb->state)
    {
    case TB_STRUCT:
      {
        return stp_build (tb);
      }
    case TB_UNION:
      {
        return unp_build (tb);
      }
    case TB_ENUM:
      {
        return enp_build (tb);
      }
    case TB_PRIM:
      {
        return SPR_DONE;
      }
    case TB_SARRAY:
      {
        return sap_build (tb);
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
typep_expect_next (const type_parser *tb, token t)
{
  switch (tb->state)
    {
    case TB_UNKNOWN:
      {
        return SBFT_TOKEN;
      }
    case TB_STRUCT:
      {
        return TYPE_ON (tb->stp, STP_WAITING_FOR_TYPE);
      }
    case TB_UNION:
      {
        return TYPE_ON (tb->unp, UNP_WAITING_FOR_TYPE);
      }
    case TB_ENUM:
      {
        return SBFT_TOKEN;
      }
    case TB_SARRAY:
      {
        return sap_expect_next (tb, t);
      }
    default:
      {
        UNREACHABLE ();
      }
    }
  return 0;
}
