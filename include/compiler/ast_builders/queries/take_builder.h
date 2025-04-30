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
    TB_WAITING_FOR_VNAME,
    TB_DONE,
  } state;

  string vname;

} take_builder;

parse_result tb_create (query_builder *dest);
parse_result tb_build (query_builder *eb);
parse_result tb_accept_token (query_builder *eb, token t);
parse_result tb_accept_type (query_builder *eb, type type);
