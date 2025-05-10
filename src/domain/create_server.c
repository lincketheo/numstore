#include "domain/create_server.h"
#include "errors/error.h"
#include "intf/types.h"

err_t
create_default_server (server *dest)
{
  err_t ret = SUCCESS;

  /**
   * For now, just create a
   * really big global allocator
   */
  lalloc alloc = lalloc_create (100000);

  error e = error_create (NULL);
  if ((ret = server_create (
           dest,
           (server_params){
               .port = 12345,
               .alloc = &alloc },
           &e)))

    {
      error_log_consume (&e);
      return ret;
    }

  return SUCCESS;
}
