#pragma once

#include "ds/llist.h"          // llnode
#include "intf/types.h"        // u32
#include "mm/lalloc.h"         // lalloc
#include "type/types.h"        // type
#include "type/types/sarray.h" // sarray_t

typedef struct
{
  u32 dim;
  llnode link;
} dim_llnode;

typedef struct
{
  llnode *head;
  type *type;

  lalloc *alloc;
} sarray_builder;

sarray_builder sab_create (lalloc *alloc);

err_t sab_accept_dim (sarray_builder *eb, u32 dim, error *e);
err_t sab_accept_type (sarray_builder *eb, type type, error *e);

err_t sab_build (sarray_t *dest, sarray_builder *eb, lalloc *destination, error *e);
