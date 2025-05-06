#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "type/types.h"

typedef struct query_builder_s query_builder;

typedef struct
{
  enum
  {
    DB_WAITING_FOR_VNAME,
    DB_DONE,
  } state;

  string vname;

} delete_builder;

stackp_result db_create (query_builder *dest);
stackp_result db_build (query_builder *eb);

// ACCEPT
stackp_result db_accept_token (query_builder *eb, token t);
