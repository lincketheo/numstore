#pragma once

#include "ds/strings.h"
#include "type/types/prim.h"
#include "type/types/struct.h"

typedef struct
{
  string *keys;
  u32 klen;
  u32 kcap;

  type *types;
  u32 tlen;
  u32 tcap;

  lalloc *alloc; // For growing types and keys arrays
} struct_builder;

err_t stb_create (struct_builder *dest, lalloc *alloc);
err_t stb_accept_key (struct_builder *eb, string key);
err_t stb_accept_type (struct_builder *eb, type t);
err_t stb_build (struct_t *dest, struct_builder *eb);
