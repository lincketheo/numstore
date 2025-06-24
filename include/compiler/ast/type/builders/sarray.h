#pragma once

#include "core/ds/llist.h"   // llnode
#include "core/intf/types.h" // u32
#include "core/mm/lalloc.h"  // lalloc

#include "compiler/ast/type/types.h"        // type
#include "compiler/ast/type/types/sarray.h" // sarray_t

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
