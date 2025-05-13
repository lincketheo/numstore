#pragma once

#include "compiler/stack_parser/common.h" // stackp_result
#include "compiler/tokens.h"              // token
#include "mm/lalloc.h"                    // lalloc

#include "type/builders/enum.h" // enum_builder

typedef struct type_parser_s type_parser;

typedef struct
{

  enum
  {
    ENP_WAITING_FOR_LB,
    ENP_WAITING_FOR_IDENT,
    ENP_WAITING_FOR_COMMA_OR_RIGHT,
    ENP_DONE,
  } state;

  enum_builder builder;

} enum_parser;

stackp_result enp_create (type_parser *dest, lalloc *alloc);

stackp_result enp_build (type_parser *eb);

stackp_result enp_accept_token (type_parser *eb, token t);
