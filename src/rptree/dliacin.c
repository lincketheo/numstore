#include "rptree/dliacin.h"

#include "dev/assert.h"
#include "errors/error.h"
#include "mm/lalloc.h"
#include "paging/page.h"
#include "paging/types/data_list.h"
#include "rptree/mem_inner_node.h"

/**
 * Yet another stateful version
 *
 * Honestly, this is just from legacy code
 * it's a bit ugly, and could deserve a refactor.
 */
typedef struct
{
  u8 *temp_buf;       // Stores the right half of the current node
  p_size tbl;         // The length of temp_buf
  mem_inner_node out; // The output internal node
  page pg0;           // Starting page
  pager *pager;       // To fetch pages
  lalloc *alloc;      // Allocates inner nodes
} dliacin_s;

typedef struct
{
  p_size idx0; // Starting local index on page
  page pg0;    // Starting page (a data list)
  pager *pager;
  lalloc *alloc;
} dliacin_s_params;

DEFINE_DBG_ASSERT_I (dliacin_s, dliacin_s, d)
{
  ASSERT (d);
  ASSERT (d->pg0.type == PG_DATA_LIST);
  if (d->tbl > 0)
    {
      ASSERT (d->temp_buf);
    }
  else
    {
      ASSERT (d->temp_buf == NULL);
    }
}

static inline err_t
dliacin_s_save_right (dliacin_s *d, p_size idx0, error *e)
{
  dliacin_s_assert (d);
  ASSERT (d->temp_buf == NULL);

  p_size used = dl_used (&d->pg0.dl);
  ASSERT (used >= idx0);
  p_size right_len = used - idx0;

  if (right_len > 0)
    {
      u8 *temp_buf = lmalloc (d->alloc, right_len, sizeof *temp_buf);
      if (temp_buf == NULL)
        {
          return error_causef (
              e, ERR_NOMEM,
              "Not enough memory to allocate "
              "%" PRp_size " bytes for temp buf in rptree",
              right_len * (p_size)sizeof *d->temp_buf);
        }

      d->temp_buf = temp_buf;

      p_size read = dl_read_out_from (&d->pg0.dl, d->temp_buf, idx0);
      ASSERT (read == right_len);
      d->tbl = read;
    }

  return SUCCESS;
}

static err_t
dliacin_s_create (dliacin_s *dest, dliacin_s_params p, error *e)
{
  ASSERT (dest);
  ASSERT (p.pg0.type == PG_DATA_LIST);

  *dest = (dliacin_s){
    .temp_buf = NULL,
    .tbl = 0,
    .out = mintn_create (p.pg0.pg),
    .pg0 = p.pg0,
    .pager = p.pager,
    .alloc = p.alloc,
  };

  err_t_wrap (dliacin_s_save_right (dest, p.idx0, e), e);

  dliacin_s_assert (dest);

  return SUCCESS;
}

static err_t
dliacin_s_alloc_then_write_once (
    dliacin_s *r,
    page *cur,
    p_size *next_write,
    const u8 *src,
    error *e)
{
  dliacin_s_assert (r);
  ASSERT (cur);
  ASSERT (*next_write > 0);

  if (dl_avail (&cur->dl) == 0)
    {
      // Allocate a new node
      page next;
      err_t_wrap (pgr_new (&next, r->pager, PG_DATA_LIST, e), e);

      // Link current node to it
      dl_set_next (&cur->dl, next.pg);

      /**
       * Push previous node's capacity (left key) and this node's
       * page number (right page number)
       */
      if (!mintn_add_right (&r->out, dl_used (&cur->dl), next.pg))
        {
          panic ();
        }

      // Commit so that we can swap it
      err_t_wrap (pgr_commit (r->pager, cur->dl.raw, cur->pg, e), e);

      // Swap it
      *cur = next;
    }

  /**
   * Write as many elements to this node as possible
   */
  *next_write = dl_write (&cur->dl, src, *next_write);

  return SUCCESS;
}

static err_t
dliacin_s_consume (dliacin_s *r, const u8 *src, t_size size, b_size n, error *e)
{
  dliacin_s_assert (r);
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n);

  b_size btotal = n * size;               // Total bytes to write
  page cur = r->pg0;                      // Current working page
  b_size written = 0;                     // Total bytes written
  pgno last_link = dl_get_next (&cur.dl); // Right node for last page

  /**
   * First,
   * Write all input data
   * and link them up correctly
   */
  while (written < btotal)
    {
      p_size next_write = btotal - written;

      err_t_wrap (
          dliacin_s_alloc_then_write_once (
              r, &cur, &next_write, src + written, e),
          e);

      ASSERT (next_write > 0); // to avoid infinite loops

      written += next_write;
    }

  /**
   * Second,
   * What's left is inside tmp_mem.
   * Write it all out (this should be max 1 new page)
   */
  written = 0;
  while (written < r->tbl)
    {
      p_size next_write = btotal - written;

      err_t_wrap (
          dliacin_s_alloc_then_write_once (
              r, &cur, &next_write, r->temp_buf + written, e),
          e);

      ASSERT (next_write > 0);

      written += next_write;
    }

  /**
   * Finally,
   * Link and Commit the current page
   */
  dl_set_next (&cur.dl, last_link);
  err_t_wrap (pgr_commit (r->pager, cur.dl.raw, cur.pg, e), e);

  return SUCCESS;
}

err_t
dliacin (mem_inner_node *dest, dliacin_params p, error *e)
{
  dliacin_s d;
  err_t ret = SUCCESS;
  dliacin_s_params dparams = {
    .pg0 = p.pg0,
    .alloc = p.alloc,
    .pager = p.pager,
    .idx0 = p.idx0,
  };

  err_t_wrap (dliacin_s_create (&d, dparams, e), e);

  err_t_wrap (dliacin_s_consume (&d, p.src, p.size, p.n, e), e);

  *dest = d.out;

  return ret;
}
