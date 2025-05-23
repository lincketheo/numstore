#include "rptree/internal/dlr.h"

// Stateful
typedef struct
{
  page *page;   // Starting page
  pager *pager; // To fetch pages
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

  page *page = pgr_get_w (p.pager, p.start);

  *dest = (dlr_s){
    .page = page,
    .pager = p.pager,
  };

  dlr_s_assert (dest);
}

static page *
dlr_s_read_once (
    dlr_s *r,
    page *cur,
    p_size *nbytes,
    u8 *dest,
    error *e)
{
  dlr_s_assert (r);
  ASSERT (cur);
  ASSERT (*nbytes > 0);
  ASSERT (!meminode_full (&r->out));

  // Nothing left - need to create a new page
  if (dl_avail (&cur->dl) == 0)
    {
      // Allocate a new node
      const page *_next = pgr_new (r->pager, PG_DATA_LIST, e);
      if (_next == NULL)
        {
          return NULL;
        }
      page *next = pgr_get_w (r->pager, _next);

      // Link current node to it
      dl_set_next (&cur->dl, next->pg);

      /**
       * Push previous node's capacity (left key) and this node's
       * page number (right page number)
       */
      meminode_push_right (&r->out, dl_used (&cur->dl), next->pg);

      // Commit so that we can swap it
      if (pgr_save (r->pager, cur, e))
        {
          return NULL;
        }

      // Swap it
      cur = next;
    }

  /**
   * Write as many elements to this node as possible
   */
  *nbytes = dl_write (&cur->dl, src, *nbytes);

  return cur;
}

static sb_size
dlr_s_write (
    dlr_s *r,
    const u8 *src,
    t_size size,
    b_size n,
    error *e)
{
  dlr_s_assert (r);
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n);

  page *cur = r->page;                     // Current working page
  b_size written = 0;                      // Total bytes written
  pgno last_link = dl_get_next (&cur->dl); // Right node for last page

  /**
   * First,
   * Write all input data
   * and link them up correctly
   */
  b_size total = r->tbl + size * n;
  b_size avail = DL_DATA_SIZE - dl_used (&r->page->dl) + DL_DATA_SIZE * meminode_avail (&r->out);

  total = MIN (total, avail);

  ASSERT (total > r->tbl); // We can fit at least 1 key - so there's more than tbl
  while (written < total - r->tbl)
    {
      p_size nbytes = total - written;

      cur = dlr_s_write_once (r, cur, &nbytes, src + written, e);
      if (cur == NULL)
        {
          return err_t_from (e);
        }

      ASSERT (nbytes > 0); // to avoid infinite loops

      written += nbytes;
    }

  ASSERT (written == total - r->tbl);

  /**
   * Second,
   * What's left is inside tmp_mem.
   * Write it all out (this should be max 1 new page)
   */
  while (written < total)
    {
      p_size nbytes = total - written;

      cur = dlr_s_write_once (r, cur, &nbytes, r->temp_buf + written, e);
      if (cur == NULL)
        {
          return err_t_from (e);
        }

      ASSERT (nbytes > 0);

      written += nbytes;
    }

  ASSERT (written == total);

  /**
   * Finally,
   * Link and Commit the current page
   */
  dl_set_next (&cur->dl, last_link);
  err_t_wrap (pgr_save (r->pager, cur, e), e);

  return written;
}

sb_size
_rpt_dlr (mem_inner_node *dest, dlr_params p, error *e)
{
  dlr_s d;

  dlr_s_create (&d, p);

  sb_size ret = dlr_s_write (&d, p.src, p.size, p.n, e);
  if (ret >= 0)
    {
      *dest = d.out;
    }

  return ret;
}
