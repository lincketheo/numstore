#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "query/builders/insert.h"
#include "type/types.h"

typedef struct query_parser_s query_parser;

typedef struct
{
  enum
  {
    IB_WAITING_FOR_VNAME,
    IB_DONE,
  } state;

  insert_builder builder;

} insert_parser;

stackp_result inp_create (query_parser *dest);

stackp_result inp_build (query_parser *eb);

stackp_result inp_accept_token (query_parser *eb, token t);
