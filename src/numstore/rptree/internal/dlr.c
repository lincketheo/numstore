#include "numstore/rptree/internal/dlr.h"

#include "numstore/paging/pager.h"          // TODO
#include "numstore/rptree/mem_inner_node.h" // TODO

typedef struct
{
  const page *page; // Starting page
  pager *pager;     // To fetch pages
} dlr_s;

DEFINE_DBG_ASSERT_I (dlr_s, dlr_s, d)
{
  ASSERT (d);
  ASSERT (d->page->type == PG_DATA_LIST);
}

static void
dlr_s_create (dlr_s *dest, dlr_params p)
{
  ASSERT (dest);
  ASSERT (p.start->type == PG_DATA_LIST);

  page *page = pgr_make_writable (p.pager, p.start);

  *dest = (dlr_s){
    .page = page,
    .pager = p.pager,
  };

  dlr_s_assert (dest);
}

static const page *
dlr_s_read_once (
    dlr_s *r,
    const page *cur,
    p_size start,
    p_size *nbytes,
    u8 *dest,
    error *e)
{
  dlr_s_assert (r);
  ASSERT (cur);
  ASSERT (*nbytes > 0);

  // Nothing left - need to create a new page
  if (dl_avail (&cur->dl) == 0)
    {
      // Allocate a new node
      const page *_next = pgr_new (r->pager, PG_DATA_LIST, e);
      if (_next == NULL)
        {
          return NULL;
        }

      // Swap it
      cur = _next;
    }

  /**
   * Write as many elements to this node as possible
   */
  *nbytes = dl_read (&cur->dl, dest, start, *nbytes);

  return cur;
}

static sb_size
dlr_s_read (
    dlr_s *r,
    u8 *dest,
    t_size size,
    b_size n,
    error *e)
{
  dlr_s_assert (r);
  ASSERT (dest);
  ASSERT (size > 0);
  ASSERT (n);

  const page *cur = r->page; // Current working page
  b_size read = 0;           // Total bytes read

  /**
   * First,
   * Write all input data
   * and link them up correctly
   */
  b_size total = size * n;

  while (read < total)
    {
      p_size nbytes = total - read;

      cur = dlr_s_read_once (r, cur, 0, &nbytes, dest + read, e);
      if (cur == NULL)
        {
          return err_t_from (e);
        }

      ASSERT (nbytes > 0); // to avoid infinite loops

      read += nbytes;
    }

  ASSERT (read == total);

  return read;
}

sb_size
_rpt_dlr (dlr_params p, error *e)
{
  dlr_s d;

  dlr_s_create (&d, p);

  return dlr_s_read (&d, p.dest, p.size, p.n, e);
}
