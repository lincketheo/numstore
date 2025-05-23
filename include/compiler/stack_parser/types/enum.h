#pragma once

#include "compiler/stack_parser/common.h" // stackp_result
#include "compiler/tokens.h"              // token
#include "mm/lalloc.h"                    // lalloc

#include "ast/type/builders/enum.h" // enum_builder

typedef struct
{

  enum
  {
    ENP_WAITING_FOR_LB,
    ENP_WAITING_FOR_IDENT,
    ENP_WAITING_FOR_COMMA_OR_RIGHT,
    ENP_DONE,
  } state;

  u32 working_start;    // Starting position to "free" [builder]
  enum_builder builder; // Builds [result]

  lalloc *destination; // Destination to allocate [result]
  enum_t result;       // Final enum - built on last token

} enum_parser;

enum_parser enp_create (lalloc *working, lalloc *destination);

stackp_result enp_accept_token (enum_parser *eb, token t, error *e);
