#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "types.h"

/////////////////////// Limited allocator

/**
 * Can allocate "dynamic" memory
 * or "const" memory (with locality)
 */

typedef struct lalloc_s lalloc;

struct lalloc_s
{
  u8 *data;
  u64 cdlen;
  u64 cdcap;

  u32 dyn_used;
  u32 dyn_limit;
};

err_t lalloc_create (lalloc *dest, u64 climit, u32 dlimit);
void lalloc_create_from (lalloc *dest, u8 *data, u64 climit, u32 dlimit);
void lalloc_free (lalloc *l);

// Const allocations
void *lmalloc_const (lalloc *a, u64 bytes);
void *lcalloc_const (lalloc *a, u64 len, u64 size);

// Dynamic allocations
void *lmalloc_dyn (lalloc *a, u32 bytes);
void *lcalloc_dyn (lalloc *a, u32 len, u32 size);
void *lrealloc_dyn (lalloc *a, void *data, u32 bytes);
void lfree_dyn (lalloc *a, void *data);
