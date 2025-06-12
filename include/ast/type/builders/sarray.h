#pragma once

#include "ast/type/types.h"        // type
#include "ast/type/types/sarray.h" // sarray_t
#include "ds/llist.h"              // llnode
#include "intf/types.h"            // u32
#include "mm/lalloc.h"             // lalloc

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
  lalloc *dest;
} sarray_builder;

sarray_builder sab_create (
    lalloc *alloc,
    lalloc *dest);

err_t sab_accept_dim (sarray_builder *eb, u32 dim, error *e);
err_t sab_accept_type (sarray_builder *eb, type type, error *e);

err_t sab_build (sarray_t *dest, sarray_builder *eb, error *e);
