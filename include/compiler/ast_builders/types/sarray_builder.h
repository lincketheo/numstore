#pragma once

#include "compiler/ast_builders/common.h"
#include "compiler/tokens.h"
#include "intf/mm.h"
#include "intf/types.h"

typedef struct type_builder_s type_builder;

typedef struct
{

  enum
  {
    SAB_WAITING_FOR_RIGHT,
    SAB_WAITING_FOR_LEFT_OR_TYPE,
    SAB_WAITING_FOR_NUMBER,
    SAB_DONE,
  } state;

  u32 *dims;
  u32 len;
  u32 cap;
  type type;

} sarray_builder;

parse_result sab_create (type_builder *dest, lalloc *alloc, u32 dim0);
parse_result sab_accept_token (type_builder *eb, token t, lalloc *alloc);
parse_result sab_accept_type (type_builder *eb, type type);
parse_result sab_build (type_builder *eb, lalloc *alloc);
parse_expect sab_expect_next (const type_builder *tb, token t);
