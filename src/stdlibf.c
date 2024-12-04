#include "stdlibf.h"

#include <errno.h>  // errno
#include <string.h> // strerror

ns_ret_t
ns_fclose (FILE *fp)
{
  ASSERT (fp);
  errno = 0;
  if (fclose (fp))
    {
      ASSERT (errno);
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
