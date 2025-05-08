#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "query/builders/create.h"
#include "type/types.h"

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
