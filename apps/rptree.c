

#include "rptree/rptree.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "paging/pager.h"
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
    .page_size = 2048,
  };
  rptree r;
  rpt_create (&r, rparams);

  int data[1000];
  int _data[1000];
  for (int i = 0; i < 1000; ++i)
    {
      data[i] = i;
    }

  // Create a new node
  pgno pg;
  werr_t (rpt_new (&r, &pg));

  // Seek to index 0
  werr_t (rpt_seek (&r, sizeof (int), 0));

  // Write to node
  werr_t (rpt_insert ((u8 *)data, sizeof (int), 1000, &r));

  // Write to node
  werr_t (rpt_insert ((u8 *)data, sizeof (int), 1000, &r));
  rpt_close (&r);

  rpt_open (&r, pg);
  b_size n = 600;
  werr_t (rpt_read ((u8 *)_data, sizeof (int), &n, 3, &r));

  fprintf (stdout, "Read: %llu\n", n);
  for (b_size i = 0; i < n; ++i)
    {
      fprintf (stdout, "%d\n", _data[i]);
    }

  rpt_close (&r);
  return SUCCESS;
}
