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
    TB_WAITING_FOR_VNAME,
    TB_DONE,
  } state;

  string vname;

} take_builder;

stackp_result tb_create (query_builder *dest);
stackp_result tb_build (query_builder *eb);

// ACCEPT
stackp_result tb_accept_token (query_builder *eb, token t);
stackp_result tb_accept_type (query_builder *eb, type type);
