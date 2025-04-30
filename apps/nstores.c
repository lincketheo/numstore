#include "database.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "server.h"
#include "server/server.h"

#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t stop = 0;

void
handle_signal (int signo)
{
  i_log_info ("Recieved signal: %d\n", signo);
  stop = 1;
}

int
main (void)
{
  err_t ret = SUCCESS;
  server s;
  lalloc cons_alloc;
  lalloc_create (&cons_alloc, 10000);

  server_params params = {
    .port = 12345,
    .cons_alloc = &cons_alloc,
  };

  if ((ret = server_create (&s, params)))
    {
      i_log_warn ("Server create failed\n");
      return ret;
    }

  while (!stop)
    {
      server_execute (&s);
      sleep (2);
    }

  server_close (&s);

  return ret;
}
