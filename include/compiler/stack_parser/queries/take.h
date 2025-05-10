#pragma once

#include "compiler/stack_parser/common.h" // stackp_result
#include "query/builders/take.h"          // take_builder

typedef struct query_parser_s query_parser;

typedef struct
{
  enum
  {
    TB_WAITING_FOR_VNAME,
    TB_DONE,
  } state;

  take_builder builder;

} take_parser;

stackp_result tkp_create (query_parser *dest);

stackp_result tkp_build (query_parser *eb);

stackp_result tkp_accept_token (query_parser *eb, token t);

stackp_result tkp_accept_type (query_parser *eb, type type);
