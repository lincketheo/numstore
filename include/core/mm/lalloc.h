#pragma once

#include "core/errors/error.h" // TODO
#include "core/intf/types.h"   // TODO

typedef struct lalloc_s
{
  u32 used;
  u32 limit;
  u8 *data;
} lalloc;

#define lalloc_create_from(buf) lalloc_create ((u8 *)buf, sizeof (buf))

lalloc lalloc_create (u8 *data, u32 limit);

u32 lalloc_get_state (const lalloc *l);

void lalloc_reset_to_state (lalloc *l, u32 state);

void *lmalloc (lalloc *a, u32 req, u32 size);

void *lcalloc (lalloc *a, u32 req, u32 size);

void lalloc_reset (lalloc *a);
