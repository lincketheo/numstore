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
    RB_WAITING_FOR_VNAME,
    RB_DONE,
  } state;

  string vname;

} read_builder;

parse_result rb_create (query_builder *dest);
parse_result rb_build (query_builder *eb);
parse_result rb_accept_token (query_builder *eb, token t);
parse_result rb_accept_type (query_builder *eb, type type);
