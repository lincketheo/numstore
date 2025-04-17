#pragma once

#include "dev/assert.h"
#include "os/types.h"

typedef struct
{
  u8 *data;
  u32 cap;
  u32 len;
} buffer;

buffer buffer_create (u8 *data, u32 cap);

static inline DEFINE_DBG_ASSERT_I (buffer, buffer, b)
{
  ASSERT (b);
  ASSERT (b->data);
  ASSERT (b->cap > 0);
  ASSERT (b->len <= b->cap);
}

static inline void
buffer_reset (buffer *b)
{
  buffer_assert (b);
  b->len = 0;
}

u32 buffer_write (const void *dest, u32 size, u32 n, buffer *b);

u32 buffer_phantom_write (u32 size, u32 n, buffer *b);
