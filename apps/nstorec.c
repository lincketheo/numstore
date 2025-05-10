
#include "client/repl.h"
#include "errors/error.h"
#include "intf/logging.h"
#include "intf/mm.h"

int
main ()
{
  repl r;
  repl_params params = {
    .port = 12345,
    .ip_addr = "127.0.0.1",
  };
  repl_create (&r, params);

  lalloc ealloc = lalloc_create (1000);
  error e = error_create (&ealloc);

  while (1)
    {
      err_t_wrap (repl_read (&r, &e), &e);
      err_t_wrap (repl_execute (&r, &e), &e);
    }
}
