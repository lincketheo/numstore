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
    CB_WAITING_FOR_VNAME,
    CB_WAITING_FOR_TYPE,
    CB_DONE,
  } state;

  string vname;
  type type;

} create_builder;

stackp_result cb_create (query_builder *dest);
stackp_result cb_build (query_builder *cb);

// ACCEPT
stackp_result cb_accept_token (query_builder *eb, token t);
stackp_result cb_accept_type (query_builder *eb, type type);
