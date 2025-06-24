#include "core/ds/strings.h"   // TODO
#include "core/errors/error.h" // TODO
#include "core/intf/io.h"      // TODO

#include "server/server.h" // TODO

int
main (void)
{
  error e = error_create (NULL);

  if (i_remove_quiet (unsafe_cstrfrom ("test.db"), &e))
    {
      error_log_consume (&e);
      return -1;
    }

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
      server_close (s, &e);
    }
  if (e.cause_code)
    {
      error_log_consume (&e);
      return -1;
    }
  return 0;
}
