#pragma once

#include "compiler/stack_parser/common.h" // stackp_result
#include "compiler/tokens.h"              // token
#include "mm/lalloc.h"                    // lalloc

#include "type/builders/enum.h" // enum_builder

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

enum_parser enp_create (lalloc *working_space);

stackp_result enp_build (enum_t *dest, enum_parser *eb, lalloc *destination, error *e);

stackp_result enp_accept_token (enum_parser *eb, token t, error *e);
