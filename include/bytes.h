#pragma once

#include "dev/assert.h"
#include "os/types.h"

typedef struct
{
  u8 *data;
  u32 cap;
} bytes;

bytes bytes_create (u8 *data, u32 cap);

static inline DEFINE_DBG_ASSERT_I (bytes, bytes, b)
{
  ASSERT (b);
  ASSERT (b->data);
  ASSERT (b->cap > 0);
}

u32 bytes_write (const void *dest, u32 size, u32 n, bytes *b);
