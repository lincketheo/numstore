#include "domain/create_server.h"

err_t
create_default_server (server *dest)
{
  err_t ret = SUCCESS;

  /**
   * For now, just create a
   * really big global allocator
   */
  lalloc alloc;
  lalloc_create (&alloc, 100000);

  /**
   * Create variable hash map
   */
  vhash_map vhm;
  if ((ret = vhash_map_create (
           &vhm, (vhm_params){
                     .len = 1000,
                     .map_allocator = &alloc,
                     .node_allocator = &alloc,
                     .type_allocator = &alloc,
                 })))
    {
      i_log_warn ("VHash Map Create Failed\n");
      return ret;
    }

  /**
   * Create the dang server
   */
  if ((ret = server_create (
           dest,
           (server_params){
               .port = 12345,
               .alloc = &alloc,
               .services = services_create (
                   (services_params){
                       .vhm = &vhm,
                   }),
           })))
    {
      i_log_warn ("Server create failed\n");
      return ret;
    }

  return SUCCESS;
}
