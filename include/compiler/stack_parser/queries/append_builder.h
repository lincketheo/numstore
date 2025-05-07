#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "services/var_retr.h"
#include "type/types.h"
#include "variables/variable.h"

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

stackp_result ab_create (query_builder *dest);
stackp_result ab_build (query_builder *eb);

// ACCEPT
stackp_result ab_accept_token (query_builder *eb, token t);
stackp_result ab_accept_type (query_builder *eb, type type);
