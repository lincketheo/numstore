#include "intf/io.h"
#include "intf/logging.h"
#include "paging/pager.h"
#include "variables/vfile_hashmap.h"
#include <stdio.h>

#define ex_log(expr)                                         \
  do                                                         \
    {                                                        \
      err_t __ret = expr;                                    \
      i_log_info ("Called: %s\n", #expr);                    \
      i_log_info ("Result was: %s\n", err_t_to_str (__ret)); \
    }                                                        \
  while (0);

int
main ()
{
  remove ("test.db");
  // Open file
  i_file fp;
  err_t ret = i_open (&fp, unsafe_cstrfrom ("test.db"), true, true);
  if (ret)
    {
      return ret;
    }

  lalloc galloc;
  lalloc_create (&galloc, 1000000);

  // Create the pager
  pgr_params pparams = {
    .page_size = 2048,
    .f = fp,
    .header_size = 0,
    .memory_pager_len = 100,
    .memory_pager_allocator = &galloc,
  };
  pager p;
  ret = pgr_create (&p, pparams);
  if (ret)
    {
      return ret;
    }

  // Create the hash table
  vfhm_params vhparams = {
    .pager = &p,
    .alloc = &galloc,
  };
  vfile_hashmap v = vfhm_create (vhparams);

  /**
   * Create the first page
   */
  ret = vfhm_create_hashmap (&v);
  if (ret)
    {
      i_log_error ("Failed to create first hash page\n");
      return ret;
    }

  vmeta meta_dest;
  vmeta m = {
    .type = { { 0 } },
    .pgn0 = 51,
  };

  char _longstr[9000];
  string longstr = {
    .data = _longstr,
    .len = 9000,
  };
  ex_log (vfhm_get (&v, &meta_dest, unsafe_cstrfrom ("foobar")));
  ex_log (vfhm_insert (&v, unsafe_cstrfrom ("foobar"), m));
  ex_log (vfhm_get (&v, &meta_dest, unsafe_cstrfrom ("foobar")));
  ex_log (vfhm_insert (&v, unsafe_cstrfrom ("foobar"), m));
  ex_log (vfhm_insert (&v, unsafe_cstrfrom ("bizb"), m));
  ex_log (vfhm_insert (&v, longstr, m));
  ex_log (vfhm_insert (&v, unsafe_cstrfrom ("foobar"), m));
  ex_log (vfhm_insert (&v, unsafe_cstrfrom ("barbzilns"), m));
  ex_log (vfhm_insert (&v, unsafe_cstrfrom ("bizb"), m));
  ex_log (vfhm_get (&v, &meta_dest, unsafe_cstrfrom ("barbzilns")));
  ex_log (vfhm_get (&v, &meta_dest, unsafe_cstrfrom ("foobar")));
  ex_log (vfhm_get (&v, &meta_dest, unsafe_cstrfrom ("bizb")));

  return SUCCESS;
}
