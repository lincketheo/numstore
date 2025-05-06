#include "rptree/dliacin.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/mm.h"
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
  pager *pager;
  lalloc *alloc;
} dliacin_s;

typedef struct
{
  p_size idx0;
  page pg0;
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
dliacin_s_save_right (dliacin_s *d, p_size idx0)
{
  dliacin_s_assert (d);
  ASSERT (d->temp_buf == NULL);

  p_size used = dl_used (&d->pg0.dl);
  ASSERT (used >= idx0);
  p_size right_len = used - idx0;

  if (right_len > 0)
    {
      d->temp_buf = lmalloc (d->alloc, right_len);
      if (d->temp_buf == NULL)
        {
          return ERR_NOMEM;
        }
      p_size read = dl_read_out_from (&d->pg0.dl, d->temp_buf, idx0);
      ASSERT (read == right_len);
      d->tbl = read;
    }

  return SUCCESS;
}

static err_t
dliacin_s_create (dliacin_s *dest, dliacin_s_params p)
{
  ASSERT (dest);
  ASSERT (p.pg0.type == PG_DATA_LIST);

  *dest = (dliacin_s){
    .temp_buf = NULL,
    .tbl = 0,
    .out = mintn_create (
        (mintn_params){
            .alloc = p.alloc,
            .pg = p.pg0.pg,
        }),
    .pg0 = p.pg0,
    .pager = p.pager,
    .alloc = p.alloc,
  };

  err_t ret = dliacin_s_save_right (dest, p.idx0);
  if (ret)
    {
      return ret;
    }

  dliacin_s_assert (dest);

  return SUCCESS;
}

static err_t
dliacin_s_alloc_then_write_once (
    dliacin_s *r,
    page *cur,
    p_size *next_write,
    const u8 *src)
{
  dliacin_s_assert (r);
  ASSERT (cur);
  ASSERT (*next_write > 0);

  err_t ret = SUCCESS;

  if (dl_avail (&cur->dl) == 0)
    {
      // Allocate a new node
      page next;
      if ((ret = pgr_new (&next, r->pager, PG_DATA_LIST)))
        {
          goto failed;
        }

      // Link current node to it
      dl_set_next (&cur->dl, next.pg);

      /**
       * Push previous node's capacity (left key) and this node's
       * page number (right page number)
       */
      if ((ret = mintn_add_right (&r->out, dl_used (&cur->dl), next.pg)))
        {
          return ret;
        }

      // Commit so that we can swap it
      if ((ret = pgr_commit (r->pager, cur->dl.raw, cur->pg)))
        {
          return ret;
        }

      // Swap it
      *cur = next;
    }

  /**
   * Write as many elements to this node as possible
   */
  *next_write = dl_write (&cur->dl, src, *next_write);

  return SUCCESS;

failed:
  return ret;
}

static err_t
dliacin_s_consume (dliacin_s *r, const u8 *src, t_size size, b_size n)
{
  dliacin_s_assert (r);
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n);

  err_t ret = SUCCESS; // Return code

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

      ret = dliacin_s_alloc_then_write_once (
          r, &cur, &next_write, src + written);

      if (ret)
        {
          goto theend;
        }

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

      ret = dliacin_s_alloc_then_write_once (
          r, &cur, &next_write, r->temp_buf + written);

      if (ret)
        {
          goto theend;
        }

      ASSERT (next_write > 0);

      written += next_write;
    }

  /**
   * Finally,
   * Link and Commit the current page
   */
  dl_set_next (&cur.dl, last_link);
  if ((ret = pgr_commit (r->pager, cur.dl.raw, cur.pg)))
    {
      goto theend;
    }

theend:
  return ret;
}

static err_t
dliacin_s_complete (mem_inner_node *dest, dliacin_s *d)
{
  dliacin_s_assert (d);
  lfree (d->alloc, d->temp_buf);

  // TODO
  // Probably don't need to clip because I think
  // later on we append more
  err_t ret = SUCCESS;
  if ((ret = mintn_clip (&d->out)))
    {
      return ret;
    }

  *dest = d->out;
  return ret;
}

err_t
dliacin (mem_inner_node *dest, dliacin_params p)
{
  dliacin_s d;
  err_t ret = SUCCESS;

  if ((ret = dliacin_s_create (
           &d, (dliacin_s_params){
                   .pg0 = p.pg0,
                   .alloc = p.alloc,
                   .pager = p.pager,
                   .idx0 = p.idx0,
               })))
    {
      return ret;
    }

  if ((ret = dliacin_s_consume (&d, p.src, p.size, p.n)))
    {
      return ret;
    }

  if ((ret = dliacin_s_complete (dest, &d)))
    {
      return ret;
    }

  return ret;
}
