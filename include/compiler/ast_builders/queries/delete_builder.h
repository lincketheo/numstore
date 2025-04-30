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
    DB_WAITING_FOR_VNAME,
    DB_DONE,
  } state;

  string vname;

} delete_builder;

parse_result db_create (query_builder *dest);
parse_result db_build (query_builder *eb);
parse_result db_accept_token (query_builder *eb, token t);
