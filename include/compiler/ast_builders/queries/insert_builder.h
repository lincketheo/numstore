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
    IB_WAITING_FOR_VNAME,
    IB_DONE,
  } state;

  string vname;

} insert_builder;

parse_result ib_create (query_builder *dest);
parse_result ib_build (query_builder *eb);
parse_result ib_accept_token (query_builder *eb, token t);
