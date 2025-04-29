
#include "client/repl.h"
#include "intf/logging.h"
int
main ()
{
  repl r;
  err_t ret;
  repl_create (&r);

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
