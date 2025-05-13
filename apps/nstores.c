#include "ds/strings.h"
#include "intf/logging.h"
#include "mm/lalloc.h"
#include "server/server.h"

#include <signal.h>
#include <stdlib.h>
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
  err_t ret = SUCCESS;

  /**
   * For now, just create a
   * really big global allocator
   */
  lalloc alloc = lalloc_create (100000);

  error e = error_create (NULL);
  if ((ret = server_create (
           &s,
           (server_params){
               .port = 12345,
               .alloc = &alloc },
           &e)))

    {
      error_log_consume (&e);
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
