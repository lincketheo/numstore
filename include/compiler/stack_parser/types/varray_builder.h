#pragma once

#include "compiler/stack_parser/common.h"
#include "intf/types.h"

typedef struct type_builder_s type_builder;

typedef struct
{

  enum
  {
    VAB_WAITING_FOR_LEFT_OR_TYPE,
    VAB_WAITING_FOR_RIGHT,
    VAB_DONE,
  } state;

  u32 rank;
  type type;

} varray_builder;

stackp_result vab_create (type_builder *dest);
stackp_result vab_build (type_builder *vab, lalloc *alloc);
sb_feed_t vab_expect_next (const type_builder *tb, token t);

// ACCEPT
stackp_result vab_accept_token (type_builder *vab, token t);
stackp_result vab_accept_type (type_builder *vab, type type);
