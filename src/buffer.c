#include "buffer.h"
#include "os/stdlib.h"

u32
buffer_write (const void *dest, u32 size, u32 n, buffer *b)
{
  n = buffer_phantom_write (size, n, b);
  i_memcpy (b->data + b->len, dest, size * n);
  return n;
}

u32
buffer_phantom_write (u32 size, u32 n, buffer *b)
{
  buffer_assert (b);
  ASSERT (size > 0);
  ASSERT (n > 0);

  ASSERT (b->cap >= b->len);
  u32 avail = b->cap - b->len;

  u32 maxn = avail / size;
  if (n > maxn)
    {
      n = maxn;
    }
  if (n == 0)
    {
      return 0;
    }

  b->len += size * n;

  return n;
}
