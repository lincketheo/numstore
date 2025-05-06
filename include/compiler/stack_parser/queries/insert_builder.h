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
    IB_WAITING_FOR_VNAME,
    IB_DONE,
  } state;

  string vname;

} insert_builder;

stackp_result ib_create (query_builder *dest);
stackp_result ib_build (query_builder *eb);

// ACCEPT
stackp_result ib_accept_token (query_builder *eb, token t);
