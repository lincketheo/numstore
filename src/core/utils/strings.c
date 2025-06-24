#include "core/utils/strings.h"

#include "core/dev/assert.h"  // TODO
#include "core/intf/stdlib.h" // TODO

u64
line_length (const char *buf, u64 max)
{
  ASSERT (buf);
  ASSERT (max > 0);

  char *nl = i_memchr (buf, '\n', max);
  u64 ret;

  if (nl != NULL)
    {
      ret = (u64)(nl - buf);
    }
  else
    {
      ret = max;
    }

  return ret;
}
