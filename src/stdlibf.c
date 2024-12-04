#include "stdlibf.h"
#include "ns_data_structures.h"
#include "ns_errors.h"
#include "ns_logging.h"
#include "ns_utils.h"

#include <errno.h> // errno
#include <stdio.h>
#include <string.h> // strerror

ns_ret_t
ns_buf_fread (buf *dest, size_t n, FILE *fp)
{
  ASSERT (dest);
  size_t toread = MIN (buf_avail (*dest), n);

  errno = 0;
  size_t read = fread (dest->data, dest->size, toread, fp);

  if (ferror (fp))
    {
      ns_log (ERROR, "Failed to read. Error: %s\n", strerror (errno));
      return errno_to_ns_error (errno);
    }

  dest->len += read;

  buf_ASSERT (dest);

  return NS_OK;
}

ns_ret_t
ns_buf_fwrite_out (buf *src, size_t n, FILE *fp)
{
  ASSERT (src);
  ASSERT (n > 0);

  size_t towrite = MIN (src->len, n);

  errno = 0;
  size_t written = fwrite (src->data, src->size, towrite, fp);
  if (written != towrite)
    {
      ns_log (ERROR, "Failed to write. Error %s\n", strerror (errno));
      return errno_to_ns_error (errno);
    }

  buf_shift_mem (src, written);

  return NS_OK;
}

ns_ret_t
ns_fclose (FILE *fp)
{
  ASSERT (fp);
  errno = 0;
  if (fclose (fp))
    {
      ASSERT (errno);
      ns_log (ERROR,
              "Failed to close file. "
              "Error: %s\n",
              strerror (errno));
      return errno_to_ns_error (errno);
    }
  return NS_OK;
}

ns_FILE_ret
ns_fopen (const char *fname, const char *mode)
{
  ASSERT (fname);
  ASSERT (mode);

  errno = 0;
  FILE *ret = fopen (fname, mode);

  if (ret == NULL)
    {
      ASSERT (errno);
      ns_log (ERROR,
              "Failed to open file: %s in mode: %s. "
              "Error: %s\n",
              fname, mode, strerror (errno));
      return (ns_FILE_ret){
        .stat = errno_to_ns_error (errno),
      };
    }

  return (ns_FILE_ret){
    .stat = NS_OK,
    .fp = ret,
  };
}
