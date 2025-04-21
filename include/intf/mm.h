#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "sds.h"
#include "types.h"

/////////////////////// Allocation

typedef struct
{
  void *data;
  err_t ret;
} alloc_ret;

// Local Allocator
typedef struct lalloc
{
  alloc_ret (*malloc) (struct lalloc *l, u64 bytes);
  alloc_ret (*calloc) (struct lalloc *l, u64 nmemb, u64 size);
  alloc_ret (*realloc) (struct lalloc *l, void *ptr, u64 bytes);
  void (*free) (struct lalloc *l, void *dest);

  // Frees all memory allocated by this allocator
  err_t (*release) (struct lalloc *l);
} lalloc;

DEFINE_DBG_ASSERT_H (lalloc, lalloc, l);

//////////////////// STDALLOC

typedef struct
{
  u64 total;
  u64 limit;
  lalloc alloc;
} stdalloc;

DEFINE_DBG_ASSERT_H (stdalloc, stdalloc, s);
stdalloc stdalloc_create (u64 limit);
