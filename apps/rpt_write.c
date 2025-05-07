

#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "paging/pager.h"
#include "rptree/rptree.h"
#include <stdio.h>

int
main ()
{
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

  // Create the rptree
  rpt_params rparams = {
    .pager = &p,
    .alloc = &galloc,
  };
  rptree r = rpt_create (rparams);

  int data[10000];
  for (int i = 0; i < 10000; ++i)
    {
      data[i] = i;
    }

  // Create a new node
  ret = rpt_new (&r);
  if (ret)
    {
      return ret;
    }

  // Seek to index 0
  ret = rpt_seek (&r, 0);
  if (ret)
    {
      return ret;
    }

  // Write to node
  ret = rpt_insert ((u8 *)data, sizeof (int), 10000, &r);
  if (ret)
    {
      return ret;
    }

  // Close
  rpt_close (&r);

  return SUCCESS;
}
