#include "dev/assert.h"
#include "intf/mm.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/////////////////////// Allocation
void *
i_malloc (u64 bytes)
{
  ASSERT (bytes > 0);
  void *ret = malloc ((size_t)bytes);
  if (ret == NULL)
    {
      i_log_error ("Failed to allocate %" PRIu64 " bytes. Reason: %s", bytes, strerror (errno));
    }
  return ret;
}

void *
i_calloc (u64 n, u64 size)
{
  ASSERT (size > 0);
  ASSERT (n > 0);
  void *ret = calloc ((size_t)n, (size_t)size);
  if (ret == NULL)
    {
      i_log_error ("Failed to c-allocate %" PRIu64 " bytes. Reason: %s", size * n,
                   strerror (errno));
    }
  return ret;
}

void
i_free (void *ptr)
{
  ASSERT (ptr);
  free (ptr);
}

void *
i_realloc (void *ptr, u64 bytes)
{
  ASSERT (ptr);
  ASSERT (bytes > 0);
  void *ret = realloc (ptr, bytes);
  if (ret == NULL)
    {
      i_log_error ("Failed to reallocate %" PRIu64 " bytes. Reason: %s", bytes,
                   strerror (errno));
    }
  return ret;
}
