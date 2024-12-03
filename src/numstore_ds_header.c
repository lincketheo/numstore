#include "numstore_ds_header.h"
#include "common_assert.h"
#include "numstore_dtype.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

int
nsdsheader_fwrite (FILE *dest, const nsdsheader src)
{
  ASSERT (dest);

  uint8_t type = src.type;
  size_t written = fwrite (&type, sizeof type, 1, dest);

  if (written != 1)
    {
      if (ferror (dest))
        {
          return -1;
        }
      unreachable ();
    }
  return 0;
}

int
nsdsheader_fread (nsdsheader *dest, FILE *src)
{
  ASSERT (src);

  uint8_t type;

  errno = 0;
  size_t read = fread (&type, sizeof type, 1, src);

  if (read != 1)
    {
      if (ferror (src))
        {
          return -1;
        }
      if (feof (src))
        {
          return -1;
        }
      unreachable ();
    }

  if (!is_dtype_valid (type))
    {
      return -1;
    }

  dest->type = type;

  return 0;
}
