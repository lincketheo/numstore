#include "compiler/ast_builders/query_builder.h"
#include "compiler/ast_builders/common.h"
#include "compiler/ast_builders/queries/create_builder.h"
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

parse_result
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
              return PR_SYNTAX_ERROR;
            }
          }
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
        return PR_SYNTAX_ERROR;
      }
    }
}

parse_result
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
        return PR_SYNTAX_ERROR;
      }
    }
}
