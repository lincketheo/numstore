#pragma once

#include "intf/types.h"

/////////////////////// Limited allocator
/**
 * Wrapper on top of a generic allocator, but has limits to
 * how much memory you can allocate.
 *
 * Returns three cases:
 * 1. I have enough memory
 * 2. I would never have enough memory
 * 3. I have enough memory but it's being used, come back later
 */
typedef struct
{
  u32 used;
  u32 limit;
} lalloc;

typedef enum
{
  AR_AVAIL_BUT_USED, // (limit - used) < min < limit
  AR_NOMEM,          // limit < min
  AR_SUCCESS,        // min < (limit - used)
} lalloc_c;

typedef struct
{
  lalloc_c stat; // Return code
  void *ret;     // The result - NULL if not AR_SUCCESS
  u32 rlen;      // Length of ret - 0 if not AR_SUCCESS
} lalloc_r;

lalloc lalloc_create (u32 limit);

lalloc_r lmalloc (lalloc *a, u32 req, u32 min, u32 size);

lalloc_r lcalloc (lalloc *a, u32 req, u32 min, u32 size);

lalloc_r lrealloc (lalloc *a, void *data, u32 req, u32 min, u32 size);

void lfree (lalloc *a, void *data);
