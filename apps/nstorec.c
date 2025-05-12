
#include "client/repl.h"
#include "errors/error.h"
#include "intf/logging.h"
#include "mm/lalloc.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile sig_atomic_t quit = 0;

void
handle_signal (int sig)
{
  if (sig == SIGINT || sig == SIGTERM)
    {
      quit = 1;
    }
}

int
main ()
{
  repl r;
  repl_params params = {
    .port = 12345,
    .ip_addr = "127.0.0.1",
  };

  error e = error_create (NULL);
  if (repl_create (&r, params, &e))
    {
      error_log_consume (&e);
      return -1;
    }

  struct sigaction sa = { 0 };
  sa.sa_handler = handle_signal;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);

  while (r.running && !quit)
    {
      if (repl_execute (&r, &e))
        {
          error_log_consume (&e);
        }
    }

  repl_release (&r);

  return 0;
}
