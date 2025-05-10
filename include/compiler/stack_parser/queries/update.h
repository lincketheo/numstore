#pragma once

#include "compiler/stack_parser/common.h" // stackp_result
#include "query/builders/update.h"        // update_builder

typedef struct query_parser_s query_parser;

typedef struct
{
  enum
  {
    UPDB_WAITING_FOR_VNAME,
    UPDB_DONE,
  } state;

  update_builder builder;

} update_parser;

stackp_result updtp_create (query_parser *dest);

stackp_result updtp_build (query_parser *eb);

stackp_result updtp_accept_token (query_parser *eb, token t);

stackp_result updtp_accept_type (query_parser *eb, type type);
