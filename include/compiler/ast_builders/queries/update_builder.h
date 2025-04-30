#pragma once

#include "compiler/ast_builders/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "typing.h"

typedef struct query_builder_s query_builder;

typedef struct
{
  enum
  {
    UB_WAITING_FOR_VNAME,
    UB_DONE,
  } state;

  string vname;

} update_builder;

parse_result updb_create (query_builder *dest);
parse_result updb_build (query_builder *eb);
parse_result updb_accept_token (query_builder *eb, token t);
parse_result updb_accept_type (query_builder *eb, type type);
