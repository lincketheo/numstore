#pragma once

#include "ast/type/types.h"               // type
#include "compiler/stack_parser/common.h" // stackp_result
#include "compiler/tokens.h"              // token

#include "ast/query/builders/create.h" // create_builder
#include "ast/query/queries/create.h"  // create_query

typedef struct
{
  enum
  {
    CB_WAITING_FOR_VNAME,
    CB_WAITING_FOR_TYPE,
    CB_DONE,
  } state;

  create_builder builder;
  create_query *dest;

} create_parser;

create_parser crtp_create (create_query *dest);
stackp_result crtp_accept_token (create_parser *p, token t, error *e);
stackp_result crtp_accept_type (create_parser *p, type type, error *e);
