#pragma once

#include "compiler/stack_parser/common.h" // stackp_result
#include "query/builders/delete.h"        // delete_builder

typedef struct query_parser_s query_parser;

typedef struct
{
  enum
  {
    DB_WAITING_FOR_VNAME,
    DB_DONE,
  } state;

  delete_builder builder;

} delete_parser;

stackp_result dltp_create (query_parser *dest);

stackp_result dltp_build (query_parser *eb);

stackp_result dltp_accept_token (query_parser *eb, token t);
