#include "domain/create_server.h"
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

  /**
   * Create variable hash map
   */

  /**
   * Create the dang server
   */
  if ((ret = server_create (
           dest,
           (server_params){
               .port = 12345,
               .alloc = &alloc },
           NULL)))

    {
      i_log_warn ("Server create failed\n");
      return ret;
    }

  return SUCCESS;
}
