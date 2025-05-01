#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "typing.h"

typedef struct type_builder_s type_builder;

typedef struct
{

  enum
  {
    UB_WAITING_FOR_LB,
    UB_WAITING_FOR_IDENT,
    UB_WAITING_FOR_TYPE,
    UB_WAITING_FOR_COMMA_OR_RIGHT,
    UB_DONE,
  } state;

  string *keys;
  type *types;
  u32 len;
  u32 cap;

} union_builder;

stackp_result ub_create (type_builder *dest, lalloc *alloc);
stackp_result ub_build (type_builder *eb, lalloc *alloc);

// ACCEPT
stackp_result ub_accept_token (type_builder *eb, token t, lalloc *alloc);
stackp_result ub_accept_type (type_builder *eb, type type);
