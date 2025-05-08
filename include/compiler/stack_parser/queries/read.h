#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "query/builders/read.h"
#include "type/types.h"

typedef struct query_parser_s query_parser;

typedef struct
{
  enum
  {
    RB_WAITING_FOR_VNAME,
    RB_DONE,
  } state;

  read_builder builder;

} read_parser;

stackp_result rdp_create (query_parser *dest);

stackp_result rdp_build (query_parser *eb);

stackp_result rdp_accept_token (query_parser *eb, token t);

stackp_result rdp_accept_type (query_parser *eb, type type);
