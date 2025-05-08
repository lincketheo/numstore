#pragma once

#include "intf/mm.h"
#include "intf/types.h"
#include "type/types.h"
#include "type/types/sarray.h"

typedef struct
{
  type *type;
  u32 *dims;
  u32 len;
  u32 cap;
  lalloc *alloc;
} sarray_builder;

err_t sab_create (sarray_builder *dest, lalloc *alloc);
err_t sab_accept_dim (sarray_builder *eb, u32 dim);
err_t sab_accept_type (sarray_builder *eb, type type);
err_t sab_build (sarray_t *dest, sarray_builder *eb);
