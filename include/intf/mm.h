#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/types.h"

/////////////////////// Limited allocator

// Limited Allocator
typedef struct
{
  u32 used;
  u32 limit;
} lalloc;

void lalloc_create (lalloc *dest, u32 limit);
void *lmalloc (lalloc *a, u32 bytes);
void *lcalloc (lalloc *a, u32 len, u32 size);
void *lrealloc (lalloc *a, void *data, u32 bytes);
void lfree (lalloc *a, void *data);
void lalloc_release (lalloc *l);

// Scoped allocator (no call alloc and free once)
typedef struct
{
  u8 *data;
  u64 len;
  u64 cap;
} salloc;

void salloc_create_from (salloc *dest, u8 *data, u64 cap);
err_t salloc_create (salloc *dest, u64 cap);
void *smalloc (salloc *a, u64 bytes);
void *scalloc (salloc *a, u64 len, u64 size);
void spop (salloc *a);

typedef enum
{
  AT_LIMITED,
  AT_SCOPED,
} alloc_t;

typedef struct
{
  alloc_t type;
  union
  {
    lalloc l;
    salloc s;
  };
} galloc;

void *gmalloc (galloc *a, u32 bytes);
void *gcalloc (galloc *a, u32 len, u32 size);
