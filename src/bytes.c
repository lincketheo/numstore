#include "bytes.h"
#include "core/numbers.h"
#include "os/stdlib.h"

bytes
bytes_create (u8 *data, u32 cap)
{
  ASSERT (data);
  ASSERT (cap > 0);
  return (bytes){
    .data = data,
    .cap = cap,
  };
}

u32
bytes_write (const void *dest, u32 size, u32 n, bytes *b)
{
  ASSERT (dest);
  ASSERT (size > 0);
  ASSERT (n > 0);
  bytes_assert (b);

  u32 ncap = b->cap / size;
  u32 ret = MIN (size * n, size * ncap);
  i_memcpy (b->data, dest, ncap);

  return ret;
}
