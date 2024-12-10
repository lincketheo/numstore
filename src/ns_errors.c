#include "ns_errors.h"
#include <errno.h>

ns_ret_t
errno_to_ns_error (int err)
{
  // Don't use this when ok
  ASSERT (err != 0);

  switch (err)
    {
    case EPERM:
    case EACCES:
      return NS_PERM;
    case ENOMEM:
      return NS_NOMEM;
    case EIO:
      return NS_IOERR;
    case EBUSY:
      return NS_BUSY;
    case EINVAL:
    case ENOTDIR:
      return NS_EINVAL;
    case ENFILE:
    case EMFILE:
      return NS_CANTOPEN;
    default:
      unreachable ();
    }
}
