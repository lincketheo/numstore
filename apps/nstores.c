#include "domain/create_server.h"
#include "ds/strings.h"
#include "intf/logging.h"
#include "intf/mm.h"
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
  server s;
  err_t ret = create_default_server (&s);
  if (ret)
    {
      return ret;
    }

  while (!stop)
    {
      server_execute (&s);
    }

  server_close (&s);

  return ret;
}
