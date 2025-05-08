#pragma once

#include "ds/strings.h"
#include "type/types/union.h"

typedef struct
{
  string *keys;
  u32 klen;
  u32 kcap;

  type *types;
  u32 tlen;
  u32 tcap;

  lalloc *alloc; // For growing keys array
} union_builder;

err_t unb_create (union_builder *dest, lalloc *alloc);
err_t unb_accept_key (union_builder *eb, string key);
err_t unb_accept_type (union_builder *eb, type t);
err_t unb_build (union_t *dest, union_builder *eb);
