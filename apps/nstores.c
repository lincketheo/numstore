#include "database.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "server/server.h"
#include "services/services.h"
#include "vhash_map.h"

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
  lalloc alloc;
  lalloc_create (&alloc, 100000);

  // Create variable hash map
  vhash_map vhm;
  if ((ret = vhash_map_create (
           &vhm, (vhm_params){
                     .len = 1000,
                     .map_allocator = &alloc,
                     .node_allocator = &alloc,
                     .type_allocator = &alloc,
                 })))
    {
      return ret;
    }

  server_params params = {
    .port = 12345,
    .alloc = &alloc,
    .services = services_create (
        (services_params){
            .vhm = &vhm,
        }),
  };

  if ((ret = server_create (&s, params)))
    {
      i_log_warn ("Server create failed\n");
      return ret;
    }

  while (!stop)
    {
      server_execute (&s);
    }

  server_close (&s);

  return ret;
}
