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
    AB_WAITING_FOR_VNAME,
    AB_DONE,
  } state;

  string vname;

} append_builder;

parse_result ab_create (query_builder *dest);
parse_result ab_build (query_builder *eb);
parse_result ab_accept_token (query_builder *eb, token t);
parse_result ab_accept_type (query_builder *eb, type type);
