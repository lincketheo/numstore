#include "intf/stdlib.h"

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
i_strncmp (char *left, char *right, u64 len)
{
  return strncmp (left, right, len);
}

int
i_unsafe_strlen (const char *cstr)
{
  return strlen (cstr);
}

int
i_memcmp (const void *s1, const void *s2, u64 n)
{
  return memcmp (s1, s2, n);
}
