#include "nsutils/page_print.h"
#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "numstore/paging/page.h"
#include "numstore/paging/pager.h"
#include "numstore/paging/types/hash_page.h"
#include "numstore/paging/types/inner_node.h"

#include <stdio.h>

static void
_page_print (pager *p, pgno pg, error *e)
{

  const page *page = pgr_get (PG_ANY, pg, p, e);
  if (page == NULL)
    {
      error_log_consume (e);
      return;
    }

  printf ("Pg: %" PRpgno "\n", pg);
  switch (page->type)
    {
    case PG_DATA_LIST:
      {
        printf ("DATA_LIST\n");
        printf ("header: %d\n", *page->dl.header);
        printf ("next: %llu\n", dl_get_next (&page->dl));
        printf ("blen: %d\n", dl_used (&page->dl));
        break;
      }
    case PG_INNER_NODE:
      {
        printf ("INNER_NODE\n");
        printf ("header: %d\n", *page->in.header);
        printf ("nkeys: %d\n", in_get_nkeys (&page->in));
        printf ("  ");
        for (u32 i = 0; i < in_get_nkeys (&page->in); ++i)
          {
            printf ("%" PRb_size "    ", in_get_key (&page->in, i));
          }
        printf ("\n");
        for (u32 i = 0; i < in_get_nkeys (&page->in) + 1; ++i)
          {
            printf ("%" PRpgno "    ", in_get_leaf (&page->in, i));
          }
        printf ("\n");
        break;
      }
    case PG_HASH_PAGE:
      {
        printf ("HASH_PAGE\n");
        printf ("header: %d\n", *page->hp.header);
        printf ("nhashes: %d\n", HP_NHASHES);
        for (u32 i = 0; i < HP_NHASHES; ++i)
          {
            if (page->hp.hashes[i] != 0)
              {
                printf ("%d: %lld\n", i, page->hp.hashes[i]);
              }
          }
        break;
      }
    case PG_HASH_LEAF:
      {
        printf ("HASH_LEAF\n");
        printf ("header: %d\n", *page->hl.header);
        printf ("next: %lld\n", hl_get_next (&page->hl));
        break;
      }
    }
}

void
page_print (char *fname, spgno pg)
{
  error e = error_create (NULL);

  pager *p = pgr_open (unsafe_cstrfrom (fname), &e);
  if (p == NULL)
    {
      error_log_consume (&e);
      return;
    }

  if (pg < 0)
    {
      for (u32 i = 0; i < pgr_get_npages (p); ++i)
        {
          printf ("=============================\n");
          _page_print (p, i, &e);
        }
      printf ("=============================\n");
    }
  else
    {
      _page_print (p, pg, &e);
    }

  pgr_close (p, &e);
}
