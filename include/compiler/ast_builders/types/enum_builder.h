#pragma once

#include "compiler/ast_builders/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "intf/mm.h"

typedef struct type_builder_s type_builder;

typedef struct
{

  enum
  {
    EB_WAITING_FOR_LB,
    EB_WAITING_FOR_IDENT,
    EB_WAITING_FOR_COMMA_OR_RIGHT,
    EB_DONE,
  } state;

  string *keys;
  u32 len;
  u32 cap;

} enum_builder;

parse_result eb_create (type_builder *dest, lalloc *alloc);
parse_result eb_accept_token (type_builder *eb, token t, lalloc *alloc);
parse_result eb_build (type_builder *eb, lalloc *alloc);
