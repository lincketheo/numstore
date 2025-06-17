#include "ds/strings.h"
#include "errors/error.h"
#include "server/server.h"

int
main ()
{
  error e = error_create (NULL);

  server *s = server_open (8123, unsafe_cstrfrom ("test.db"), &e);

  if (s == NULL)
    {
      goto theend;
    }

  while (!server_is_done (s))
    {
      server_execute (s);
    }

theend:
  if (s)
    {
      server_close (s);
    }
  if (e.cause_code)
    {
      error_log_consume (&e);
      return -1;
    }
  return 0;
}
