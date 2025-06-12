#include "math/random.h"
#include "dev/assert.h"

#include <stdlib.h>
#include <time.h>

void
rand_seed (void)
{
  srand (time (NULL));
}

void
rand_seed_with (u32 seed)
{
  srand (seed);
}

u32
randu32 (u32 lower, u32 upper)
{
  if (upper <= lower)
    {
      return lower;
    }

  return lower + (u32)((rand () % (upper - lower)));
}

err_t
rand_str (string *dest, lalloc *alloc, u32 minlen, u32 maxlen, error *e)
{
  ASSERT (dest);
  ASSERT (maxlen > minlen);

  u32 len = randu32 (minlen, maxlen);
  char *data = (char *)lmalloc (alloc, len, sizeof *data);
  if (!data)
    {
      return error_causef (e, ERR_NOMEM, "%s", "rand_str allocation failed");
    }

  for (u32 i = 0; i < len; ++i)
    {
      // Choose a printable ASCII character from [a-zA-Z0-9]
      int r = randu32 (0, 62);
      if (r < 10)
        {
          data[i] = '0' + r;
        }
      else if (r < 36)
        {
          data[i] = 'A' + (r - 10);
        }
      else
        {
          data[i] = 'a' + (r - 36);
        }
    }

  dest->len = (u16)len;
  dest->data = data;

  return SUCCESS;
}
