#include "posixf.h"
#include "ns_errors.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

ns_ret_t
ns_mkdir (const char *name, mode_t mode)
{
  errno = 0;
  if (mkdir (name, 0755))
    {
      ASSERT (errno);
      ns_log (ERROR,
              "Failed to create directory %s. "
              "Error: %s\n",
              name, strerror (errno));
      return errno_to_ns_error (errno);
    }

  return NS_OK;
}
