#pragma once

#include "compiler/stack_parser/common.h" // stackp_result
#include "query/builders/append.h"        // append_builder

typedef struct query_parser_s query_parser;

typedef struct
{
  enum
  {
    AB_WAITING_FOR_VNAME,
    AB_DONE,
  } state;

  append_builder builder;

} append_parser;

stackp_result apnd_create (query_parser *dest);

stackp_result apnd_build (query_parser *eb);

stackp_result apnd_accept_token (query_parser *eb, token t);

stackp_result apnd_accept_type (query_parser *eb, type type);
