

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
  werr_t (i_open (&fp, unsafe_cstrfrom ("test.db"), true, true));

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
  werr_t (pgr_create (&p, pparams));

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
  werr_t (rpt_new (&r));

  // Seek to index 0
  werr_t (rpt_seek (&r, 0));

  // Write to node
  werr_t (rpt_insert ((u8 *)data, sizeof (int), 10000, &r));

  // Close
  rpt_close (&r);

  return SUCCESS;
}
