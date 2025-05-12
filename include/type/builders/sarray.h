#pragma once

#include "mm/lalloc.h"
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

err_t sab_create (sarray_builder *dest, lalloc *alloc, error *e);
err_t sab_accept_dim (sarray_builder *eb, u32 dim, error *e);
err_t sab_accept_type (sarray_builder *eb, type type, error *e);
err_t sab_build (sarray_t *dest, sarray_builder *eb, error *e);
