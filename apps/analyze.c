
#include "core/ds/strings.h"   // TODO
#include "core/errors/error.h" // TODO

#include "numstore/paging/page.h"  // TODO
#include "numstore/paging/pager.h" // TODO
#include "numstore/paging/types/data_list.h"
#include <stdio.h>

u32 data[2048];
u8 *raw = (u8 *)data;

int
main (void)
{
  error e = error_create (NULL);
  pager *p = pgr_open (unsafe_cstrfrom ("test.db"), &e);

  if (p == NULL)
    {
      error_log_consume (&e);
      return -1;
    }

  for (u32 i = 0; i < pgr_get_npages (p); ++i)
    {
      const page *pg = pgr_get (PG_ANY, i, p, &e);
      switch (pg->pg)
        {
        case 1:
        case 3:
        case 5:
        case 6:
        case 7:
          {
            dl_read (&pg->dl, raw, 0, dl_used (&pg->dl));
            raw += dl_used (&pg->dl);
            break;
          }
        default:
          continue;
        }
      if (pg == NULL)
        {
          error_log_consume (&e);
          return -1;
        }
    }

  for (u32 i = 0; i < 2047; ++i)
    {
      printf ("%d\n", data[i]);
      ASSERT (data[i + 1] == data[i] + 1);
    }

  pgr_close (p, &e);

  return 0;
}
