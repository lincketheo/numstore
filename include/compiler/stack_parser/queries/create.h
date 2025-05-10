#pragma once

#include "compiler/stack_parser/common.h" // stackp_result
#include "query/builders/create.h"        // create_builder

typedef struct query_parser_s query_parser;

typedef struct
{
  enum
  {
    CB_WAITING_FOR_VNAME,
    CB_WAITING_FOR_TYPE,
    CB_DONE,
  } state;

  create_builder builder;

} create_parser;

stackp_result crtp_create (query_parser *dest);

stackp_result crtp_build (query_parser *cb);

stackp_result crtp_accept_token (query_parser *eb, token t);

stackp_result crtp_accept_type (query_parser *eb, type type);
