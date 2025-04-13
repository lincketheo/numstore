#include "os/stdlib.h"

#include <stddef.h>
#include <string.h>

void *
i_memset (void *s, int c, u64 n)
{
  return memset (s, c, (size_t)n);
}

void *
i_memcpy (void *dest, const void *src, u64 bytes)
{
  return memcpy (dest, src, bytes);
}

int
i_unsafe_strlen (const char *cstr)
{
  return strlen (cstr);
}
