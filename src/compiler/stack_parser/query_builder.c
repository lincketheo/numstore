#include "compiler/stack_parser/query_builder.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/queries/create_builder.h"
#include "compiler/tokens.h"

DEFINE_DBG_ASSERT_I (query_builder, query_builder, qb)
{
  ASSERT (qb);
}

query_builder
qb_create (void)
{
  return (query_builder){
    .state = QB_UNKNOWN,
  };
}

stackp_result
qb_build (query_builder *qb)
{
  query_builder_assert (qb);

  switch (qb->state)
    {
    case QB_CREATE:
      {
        return cb_build (qb);
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
qb_expect_next (const query_builder *qb)
{
  switch (qb->state)
    {
    case QB_UNKNOWN:
      {
        return SBFT_TOKEN;
      }
    case QB_CREATE:
      {
        return TYPE_ON (qb->cb, CB_WAITING_FOR_TYPE);
      }
    case QB_DELETE:
      {
        return SBFT_TOKEN;
      }
    case QB_APPEND:
      {
        return SBFT_TOKEN;
      }
    case QB_UPDATE:
      {
        return SBFT_TOKEN;
      }
    case QB_READ:
      {
        return SBFT_TOKEN;
      }
    case QB_TAKE:
      {
        return SBFT_TOKEN;
      }
    case QB_INSERT:
      {
        return SBFT_TOKEN;
      }
    }
  return SBFT_TOKEN;
}

stackp_result
qb_accept_token (query_builder *qb, token t)
{
  query_builder_assert (qb);

  switch (qb->state)
    {
    case QB_UNKNOWN:
      {
        switch (t.type)
          {
          case TT_CREATE:
            {
              return cb_create (qb);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    case QB_CREATE:
      {
        return cb_accept_token (qb, t);
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
qb_accept_type (query_builder *qb, type t)
{
  query_builder_assert (qb);
  switch (qb->state)
    {
    case QB_CREATE:
      {
        return cb_accept_type (qb, t);
      }
    default:
      {
        return SPR_SYNTAX_ERROR;
      }
    }
}
