#include "client/repl.h"
#include "errors/error.h"

int
main (void)
{
  error e = error_create (NULL);

  repl *r = repl_open (
      (repl_params){
          .ip_addr = "127.0.0.1",
          .port = 8123,
      },
      &e);

  if (r == NULL)
    {
      goto theend;
    }

  while (repl_is_running (r))
    {
      if (repl_execute (r, &e))
        {
          goto theend;
        }
    }

theend:
  if (r)
    {
      repl_close (r);
    }
  if (e.cause_code)
    {
      error_log_consume (&e);
      return -1;
    }
  return 0;
}
