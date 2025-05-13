#pragma once

#include "errors/error.h"
#include "intf/types.h"

/////////////////////// Linear Allocator

typedef struct lalloc_s
{
  u32 used;
  u32 limit;
  u8 *data;
} lalloc;

#define lalloc_create_from(buf) lalloc_create ((u8 *)buf, sizeof (buf))

lalloc lalloc_create (u8 *data, u32 limit);

lalloc lalloc_reserve_remaining (lalloc *from);

err_t lalloc_reserve (lalloc *dest, lalloc *from, u32 amount, error *e);

void *lmalloc (lalloc *a, u32 req, u32 size);

void *lcalloc (lalloc *a, u32 req, u32 size);

void lalloc_reset (lalloc *a);
