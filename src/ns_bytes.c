#include "ns_bytes.h"

#include <string.h> // memcpy

///////////////////////////////
////////// Bytes

void
ns_memcpy (bytes *dest, const bytes *src, ns_size size)
{
  ASSERT (dest);
  ASSERT (src);
  ASSERT (dest->cap >= size);
  ASSERT (src->cap >= size);
  memcpy (dest->data, src->data, size);
}

void
ns_memmove (bytes *dest, const bytes *src, ns_size size)
{
  ASSERT (dest);
  ASSERT (src);
  ASSERT (dest->cap >= size);
  ASSERT (src->cap >= size);
  memmove (dest->data, src->data, size);
}

bytes
bytes_from (const bytes src, ns_size from)
{
  ASSERT (!bytes_IS_FREE (&src));
  ASSERT (from <= src.cap);
  return bytes_create_from (&src.data[from], src.cap - from);
}
