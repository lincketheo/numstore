#pragma once

#include "compiler/ast_builders/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "typing.h"

typedef struct type_builder_s type_builder;

typedef struct
{

  enum
  {
    SB_WAITING_FOR_LB,
    SB_WAITING_FOR_IDENT,
    SB_WAITING_FOR_TYPE,
    SB_WAITING_FOR_COMMA_OR_RIGHT,
    SB_DONE,
  } state;

  string *keys;
  type *types;
  u32 len;
  u32 cap;

} struct_builder;

parse_result sb_create (type_builder *dest, lalloc *alloc);
parse_result sb_accept_token (type_builder *eb, token t, lalloc *alloc);
parse_result sb_accept_type (type_builder *eb, type type);
parse_result sb_build (type_builder *eb, lalloc *alloc);
parse_expect sb_expect_next (const type_builder *tb);
