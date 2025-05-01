#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "typing.h"

typedef struct query_builder_s query_builder;

typedef struct
{
  enum
  {
    UPDB_WAITING_FOR_VNAME,
    UPDB_DONE,
  } state;

  string vname;

} update_builder;

stackp_result updb_create (query_builder *dest);
stackp_result updb_build (query_builder *eb);

// ACCEPT
stackp_result updb_accept_token (query_builder *eb, token t);
stackp_result updb_accept_type (query_builder *eb, type type);
