#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "type/builders/struct.h"
#include "type/types.h"

typedef struct type_parser_s type_parser;

typedef struct
{

  enum
  {
    STP_WAITING_FOR_LB,
    STP_WAITING_FOR_IDENT,
    STP_WAITING_FOR_TYPE,
    STP_WAITING_FOR_COMMA_OR_RIGHT,
    STP_DONE,
  } state;

  struct_builder builder;

} struct_parser;

stackp_result stp_create (type_parser *dest, lalloc *alloc);

stackp_result stp_build (type_parser *sb);

stackp_result stp_accept_token (type_parser *eb, token t);

stackp_result stp_accept_type (type_parser *eb, type type);
