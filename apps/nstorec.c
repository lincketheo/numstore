
#include "client/repl.h"
#include "intf/logging.h"

int
main ()
{
  repl r;
  err_t ret;
  repl_params params = {
    .port = 12345,
    .ip_addr = "127.0.0.1",
  };
  repl_create (&r, params);

  while (1)
    {
      if ((ret = repl_read (&r)))
        {
          i_log_error ("read error\n");
          return ret;
        }
      if ((ret = repl_execute (&r)))
        {
          i_log_error ("execute error\n");
          return ret;
        }
    }
}
