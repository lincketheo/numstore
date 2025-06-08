#include "compiler/stack_parser/ast_parser.h"
#include "ast/type/types.h"
#include "compiler/stack_parser/common.h"
#include "dev/assert.h"

sb_feed_t
ast_parser_expect_next (ast_parser *b, token t)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        switch (b->tb.state)
          {
          case TB_UNKNOWN:
            {
              return SBFT_TOKEN;
            }
          case TB_STRUCT:
          case TB_UNION:
            {
              if (b->tb.kvp.state == KVTP_WAITING_FOR_TYPE)
                {
                  return SBFT_TYPE;
                }
              else
                {
                  return SBFT_TOKEN;
                }
            }
          case TB_ENUM:
            {
              return SBFT_TOKEN;
            }
          case TB_SARRAY:
            {
              if (b->tb.sap.state == SAP_WAITING_FOR_LEFT_OR_TYPE && t.type == TT_LEFT_BRACKET)
                {
                  return SBFT_TOKEN;
                }
              else
                {
                  return SBFT_TYPE;
                }
            }
          default:
            {
              UNREACHABLE ();
            }
          }
      }
    case SBBT_QUERY:
      {
        query_parser *qb = &b->qb;
        switch (qb->state)
          {
          case QP_UNKNOWN:
            {
              return SBFT_TOKEN;
            }
          case QP_CREATE:
            {
              if (qb->cp.state == CB_WAITING_FOR_TYPE)
                {
                  return SBFT_TYPE;
                }
              else
                {
                  return SBFT_TOKEN;
                }
            }
          case QP_DELETE:
            {
              return SBFT_TOKEN;
            }
          }
        return SBFT_TOKEN;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

ast_result
ast_parser_to_result (ast_parser *b)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        switch (b->tb.state)
          {
          case TB_STRUCT:
            {
              return (ast_result){
                .type = SBBT_TYPE,
                .t = (type){
                    .type = T_STRUCT,
                    .st = b->tb.kvp.result.st_res,
                },
              };
            }
          case TB_UNION:
            {
              return (ast_result){
                .type = SBBT_TYPE,
                .t = (type){
                    .type = T_UNION,
                    .un = b->tb.kvp.result.un_res,
                },
              };
            }
          case TB_ENUM:
            {
              return (ast_result){
                .type = SBBT_TYPE,
                .t = (type){
                    .type = T_ENUM,
                    .en = b->tb.enp.result,
                },
              };
            }
          case TB_SARRAY:
            {
              return (ast_result){
                .type = SBBT_TYPE,
                .t = (type){
                    .type = T_SARRAY,
                    .sa = b->tb.sap.result,
                },
              };
            }
          case TB_PRIM:
            {
              return (ast_result){
                .type = SBBT_TYPE,
                .t = (type){
                    .type = T_PRIM,
                    .p = b->tb.p,
                },
              };
            }
          default:
            {
              UNREACHABLE ();
            }
          }
      }
    case SBBT_QUERY:
      {
        return (ast_result){
          .type = SBBT_QUERY,
          .q = b->cur,
        };
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
ast_parser_accept_type (ast_parser *b, type t, error *e)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        switch (b->tb.state)
          {
          case TB_STRUCT:
          case TB_UNION:
            {
              return kvp_accept_type (&b->tb.kvp, t, e);
            }
          case TB_SARRAY:
            {
              return sap_accept_type (&b->tb.sap, t, e);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    case SBBT_QUERY:
      {
        switch (b->qb.state)
          {
          case QP_CREATE:
            {
              return crtp_accept_type (&b->qb.cp, t, e);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
ast_parser_accept_token (
    ast_parser *b,
    token t,
    lalloc *alloc,
    error *e)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        switch (b->tb.state)
          {
          case TB_UNKNOWN:
            {
              switch (t.type)
                {
                case TT_STRUCT:
                  {
                    b->tb.state = TB_STRUCT;
                    b->tb.kvp = kvp_create (
                        alloc,
                        b->cur.qalloc,
                        T_STRUCT);
                    return SPR_CONTINUE;
                  }
                case TT_UNION:
                  {
                    b->tb.state = TB_UNION;
                    b->tb.kvp = kvp_create (
                        alloc,
                        b->cur.qalloc,
                        T_UNION);
                    return SPR_CONTINUE;
                  }
                case TT_ENUM:
                  {
                    b->tb.state = TB_ENUM;
                    b->tb.enp = enp_create (alloc, b->cur.qalloc);
                    return SPR_CONTINUE;
                  }
                case TT_LEFT_BRACKET:
                  {
                    b->tb.state = TB_SARRAY;
                    b->tb.sap = sap_create (alloc, b->cur.qalloc);
                    return SPR_CONTINUE;
                  }
                case TT_PRIM:
                  {
                    b->tb.p = t.prim;
                    b->tb.state = TB_PRIM;
                    return SPR_DONE;
                  }
                default:
                  {
                    return SPR_SYNTAX_ERROR;
                  }
                }
            }
          case TB_STRUCT:
          case TB_UNION:
            {
              return kvp_accept_token (&b->tb.kvp, t, e);
            }
          case TB_ENUM:
            {
              return enp_accept_token (&b->tb.enp, t, e);
            }
          case TB_SARRAY:
            {
              return sap_accept_token (&b->tb.sap, t, e);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    case SBBT_QUERY:
      {
        switch (b->qb.state)
          {
          case QP_UNKNOWN:
            {
              switch (t.type)
                {
                case TT_CREATE:
                  {
                    b->qb.state = QP_CREATE;
                    b->qb.cp = crtp_create (t.q.create);
                    b->cur = t.q;

                    return SPR_CONTINUE;
                  }
                case TT_DELETE:
                  {
                    b->qb.state = QP_DELETE;
                    b->qb.dp = dltp_create (t.q.delete);
                    b->cur = t.q;

                    return SPR_CONTINUE;
                  }
                default:
                  {
                    return SPR_SYNTAX_ERROR;
                  }
                }
            }
          case QP_CREATE:
            {
              return crtp_accept_token (&b->qb.cp, t, e);
            }
          case QP_DELETE:
            {
              return dltp_accept_token (&b->qb.dp, t, e);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}
