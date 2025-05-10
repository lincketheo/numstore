#pragma once

#include "dev/assert.h"
#include "dev/testing.h"
#include "intf/types.h"

/////////////////////// Limited allocator

// Limited Allocator
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
  lalloc_c stat;
  void *ret; // The result - NULL if not AR_SUCCESS
  u32 rlen;  // Length of ret - 0 if not AR_SUCCESS
} lalloc_r;

lalloc lalloc_create (u32 limit);

lalloc_r lmalloc (lalloc *a, u32 req, u32 min, u32 size);

lalloc_r lcalloc (lalloc *a, u32 req, u32 min, u32 size);

lalloc_r lrealloc (lalloc *a, void *data, u32 req, u32 min, u32 size);

void lfree (lalloc *a, void *data);

#ifndef NTEST
/**
 * For tests and debugging, I just want an easy wrapper
 * that has the same behavior as malloc
 */
void *lmalloc_test (lalloc *a, u32 n, u32 size);
void *lcalloc_test (lalloc *a, u32 n, u32 size);
#endif
