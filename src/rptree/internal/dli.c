#include "rptree/internal/dli.h"
#include "config.h"
#include "paging/pager.h"
#include "paging/types/data_list.h"
#include "rptree/mem_inner_node.h"
#include "utils/macros.h"

// Stateful
typedef struct
{
  u8 temp_buf[PAGE_SIZE]; // Stores the right half of the current node
  p_size tbl;             // The length of temp_buf
  mem_inner_node out;     // The output internal node
  page *page;             // Starting page
  pager *pager;           // To fetch pages
} dli_s;

DEFINE_DBG_ASSERT_I (dli_s, dli_s, d)
{
  ASSERT (d);
  ASSERT (d->page->type == PG_DATA_LIST);
}

// Save right half of [d] current page to tempbuf
static inline void
dli_s_save_right (dli_s *d, p_size idx0)
{
  dli_s_assert (d);
  ASSERT (d->temp_buf == NULL);

  p_size used = dl_used (&d->page->dl);
  ASSERT (used >= idx0);
  p_size right_len = used - idx0;

  if (right_len > 0)
    {
      p_size read = dl_read_out_from (&d->page->dl, d->temp_buf, idx0);
      ASSERT (read == right_len);
      d->tbl = read;
    }
}

static void
dli_s_create (dli_s *dest, dli_params p)
{
  ASSERT (dest);
  ASSERT (p.start->type == PG_DATA_LIST);

  page *page = pgr_get_w (p.pager, p.start);

  *dest = (dli_s){
    // temp_buf, tbl = dli_save_right
    // out = meminode_create
    .page = page,
    .pager = p.pager,
  };

  meminode_create (&dest->out, p.start->pg);

  dli_s_save_right (dest, p.idx0);

  dli_s_assert (dest);
}

static page *
dli_s_write_once (
    dli_s *r,
    page *cur,
    p_size *nbytes,
    const u8 *src,
    error *e)
{
  dli_s_assert (r);
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
dli_s_write (
    dli_s *r,
    const u8 *src,
    t_size size,
    b_size n,
    error *e)
{
  dli_s_assert (r);
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

      cur = dli_s_write_once (r, cur, &nbytes, src + written, e);
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

      cur = dli_s_write_once (r, cur, &nbytes, r->temp_buf + written, e);
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
_rpt_dli (mem_inner_node *dest, dli_params p, error *e)
{
  dli_s d;

  dli_s_create (&d, p);

  sb_size ret = dli_s_write (&d, p.src, p.size, p.n, e);
  if (ret >= 0)
    {
      *dest = d.out;
    }

  return ret;
}
