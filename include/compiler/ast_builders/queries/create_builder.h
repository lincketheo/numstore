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
    CB_WAITING_FOR_VNAME,
    CB_WAITING_FOR_TYPE,
    CB_DONE,
  } state;

  string vname;
  type type;

} create_builder;

parse_result cb_create (query_builder *dest);
parse_result cb_build (query_builder *eb);
parse_result cb_accept_token (query_builder *eb, token t);
parse_result cb_accept_type (query_builder *eb, type type);
