
#pragma once

#include "ast/value/values/sarray.h" // sarray_v
#include "mm/lalloc.h"               // lalloc

typedef struct
{
  value *values;
  u32 cap;
  u32 len;

  lalloc *alloc;
} sarrayv_builder;

err_t savb_create (sarrayv_builder *dest, lalloc *alloc);
err_t savb_accept_value (sarrayv_builder *eb, value v);
err_t savb_build (sarray_v *dest, sarrayv_builder *eb);
