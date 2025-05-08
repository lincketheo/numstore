#pragma once

#include "ds/strings.h"
#include "value/values/struct.h"

typedef struct
{
  string *keys;
  u32 kcap;
  u32 klen;

  value *values;
  u32 vcap;
  u32 vlen;

  lalloc *alloc;
} structv_builder;

err_t stvb_create (structv_builder *dest, lalloc *alloc);
err_t stvb_accept_key (structv_builder *eb, string key);
err_t stvb_accept_value (structv_builder *eb, value v);
err_t stvb_build (struct_v *dest, structv_builder *eb);
