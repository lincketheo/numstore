#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "types.h"

/////////////////////// Limited allocator

typedef struct
{
  u64 limit;
  u64 total;
} lalloc;

DEFINE_DBG_ASSERT_H (lalloc, lalloc, l);
lalloc lalloc_create (u64 limit);
void *lmalloc (lalloc *a, u64 bytes);
void *lrealloc (lalloc *a, void *data, u64 bytes);
void lfree (lalloc *a, void *data);
