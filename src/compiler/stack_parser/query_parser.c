#include "compiler/stack_parser/query_parser.h"
#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"

DEFINE_DBG_ASSERT_I (query_parser, query_parser, qb)
{
  ASSERT (qb);
}

query_parser
qryp_create (void)
{
  return (query_parser){
    .state = QP_UNKNOWN,
  };
}

stackp_result
qryp_build (query_parser *qb)
{
  query_parser_assert (qb);

  switch (qb->state)
    {
    case QP_CREATE:
      {
        return crtp_build (qb);
      }
    case QP_APPEND:
      {
        return apnd_build (qb);
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
qryp_expect_next (const query_parser *qb)
{
  switch (qb->state)
    {
    case QP_UNKNOWN:
      {
        return SBFT_TOKEN;
      }
    case QP_CREATE:
      {
        return TYPE_ON (qb->cp, CB_WAITING_FOR_TYPE);
      }
    case QP_DELETE:
      {
        return SBFT_TOKEN;
      }
    case QP_APPEND:
      {
        return SBFT_TOKEN;
      }
    case QP_UPDATE:
      {
        return SBFT_TOKEN;
      }
    case QP_READ:
      {
        return SBFT_TOKEN;
      }
    case QP_TAKE:
      {
        return SBFT_TOKEN;
      }
    case QP_INSERT:
      {
        return SBFT_TOKEN;
      }
    }
  return SBFT_TOKEN;
}

stackp_result
qryp_accept_token (query_parser *qb, token t)
{
  query_parser_assert (qb);

  switch (qb->state)
    {
    case QP_UNKNOWN:
      {
        switch (t.type)
          {
          case TT_CREATE:
            {
              return crtp_create (qb);
            }
          case TT_APPEND:
            {
              return apnd_create (qb);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    case QP_CREATE:
      {
        return crtp_accept_token (qb, t);
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
qryp_accept_type (query_parser *qb, type t)
{
  query_parser_assert (qb);
  switch (qb->state)
    {
    case QP_CREATE:
      {
        return crtp_accept_type (qb, t);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
