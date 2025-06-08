#pragma once

#include "ast/type/types.h"               // type
#include "compiler/stack_parser/common.h" // stackp_result
#include "compiler/tokens.h"              // token

#include "ast/query/builders/delete.h" // delete_builder
#include "ast/query/queries/delete.h"  // delete_query

typedef struct
{
  enum
  {
    DB_WAITING_FOR_VNAME,
    DB_DONE,
  } state;

  delete_builder builder;
  delete_query *dest;

} delete_parser;

delete_parser dltp_create (delete_query *dest);
stackp_result dltp_accept_token (delete_parser *eb, token t, error *e);
