#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "type/builders/union.h"
#include "type/types.h"

typedef struct type_parser_s type_parser;

typedef struct
{

  enum
  {
    UNP_WAITING_FOR_LB,
    UNP_WAITING_FOR_IDENT,
    UNP_WAITING_FOR_TYPE,
    UNP_WAITING_FOR_COMMA_OR_RIGHT,
    UNP_DONE,
  } state;

  union_builder builder;

} union_parser;

stackp_result unp_create (type_parser *dest, lalloc *alloc);

stackp_result unp_build (type_parser *eb);

stackp_result unp_accept_token (type_parser *eb, token t);

stackp_result unp_accept_type (type_parser *eb, type type);
