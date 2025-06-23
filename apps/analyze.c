
#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "numstore/paging/page.h"
#include "numstore/paging/pager.h"
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
      if (pg == NULL)
        {
          error_log_consume (&e);
          return -1;
        }
      i_log_page (pg);
    }

  pgr_close (p, &e);

  return 0;
}
