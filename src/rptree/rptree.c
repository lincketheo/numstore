#include "rptree/rptree.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "paging/types/data_list.h"
#include "paging/types/inner_node.h"
#include <stdlib.h>

DEFINE_DBG_ASSERT_I (rptree, rptree, r)
{
  ASSERT (r);
  if (!r->is_open)
    {
      ASSERT (!r->is_seeked);
    }
  if (r->is_seeked)
    {
      ASSERT (r->cur.type == PG_DATA_LIST);
      ASSERT (r->lidx <= *r->cur.dl.blen);
    }
}

err_t
rpt_create (rptree *dest, rpt_params params)
{
  ASSERT (dest);

  u8 *temp_mem = lmalloc (params.alloc, params.page_size);
  if (temp_mem == NULL)
    {
      return ERR_NOMEM;
    }

  *dest = (rptree){
    .pager = params.pager,
    .is_open = false,
    .is_seeked = false,
    .temp_mem = temp_mem,
    .tmlen = 0,
    .tmcap = params.page_size,
    .alloc = params.alloc,
  };

  return SUCCESS;
}

err_t
rpt_new (rptree *r, pgno *p0)
{
  rptree_assert (r);
  ASSERT (!r->is_open);
  ASSERT (p0);

  werr_t (pgr_new (&r->cur, r->pager, PG_DATA_LIST));

  return rpt_open (r, r->cur.pg);
}

err_t
rpt_open (rptree *r, pgno p0)
{
  rptree_assert (r);
  ASSERT (!r->is_open);

  // Fetch the root page
  werr_t (pgr_get_expect (
      &r->cur,
      PG_INNER_NODE | PG_DATA_LIST,
      p0, r->pager));

  r->gidx = 0;
  r->lidx = 0;
  r->is_open = true;
  ASSERT (!r->is_seeked);

  return SUCCESS;
}

void
rpt_close (rptree *r)
{
  rptree_assert (r);
  ASSERT (r->is_open);
  r->is_open = false;
  r->is_seeked = false;
}

static err_t
rpt_seek_recursive (rptree *r, b_size byte)
{
  rptree_assert (r);

  switch (r->cur.type)
    {
    case PG_INNER_NODE:
      {
        // Choose which leaf to traverse to next
        b_size left;
        pgno next = in_choose_leaf (&r->cur.in, &left, byte);

        // Subtract left quantity from byte query
        ASSERT (byte >= left);
        r->gidx += left;
        byte -= left;

        // Fetch the next page
        werr_t (pgr_get_expect (
            &r->cur,
            PG_INNER_NODE | PG_DATA_LIST,
            next, r->pager));

        // Recursive call
        return rpt_seek_recursive (r, byte);
      }
    case PG_DATA_LIST:
      {
        if ((p_size)byte >= *r->cur.dl.blen)
          {
            r->lidx = *r->cur.dl.blen;
          }
        else
          {
            r->lidx = byte;
          }
        r->gidx += r->lidx;
        r->is_seeked = true;
        return SUCCESS;
      }
    default:
      {
        return ERR_INVALID_STATE;
      }
    }
}

err_t
rpt_seek (rptree *r, t_size size, b_size n)
{
  rptree_assert (r);
  ASSERT (r->is_open);
  ASSERT (!r->is_seeked);

  b_size byte = size * n;

  return rpt_seek_recursive (r, byte);
}

static err_t
rpt_append_here (const u8 *buf, b_size len, rptree *r)
{
  rptree_assert (r);

  b_size written = 0;
  while (written < len)
    {
      if (dl_avail (&r->cur.dl) == 0)
        {
          // Create a new page
          page next;
          werr_t (pgr_new (&next, r->pager, PG_DATA_LIST));

          // Link current page to new page
          dl_set_next (&r->cur.dl, next.pg);
          werr_t (pgr_commit (r->pager, r->cur.dl.raw, r->cur.pg));

          // Update current page
          r->cur = next;
          r->lidx = 0;
        }

      // Append to current page
      p_size chunk = len - written;
      p_size w = dl_write (&r->cur.dl, buf + written, chunk);

      written += w;

      // Stay on track with writes
      r->lidx += w;
      r->gidx += w;
    }
  return SUCCESS;
}

err_t
rpt_insert (const u8 *src, t_size size, b_size n, rptree *r)
{
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n > 0);
  rptree_assert (r);
  ASSERT (r->is_seeked);

  // 1) extract and save tail
  ASSERT (r->tmlen == 0);
  r->tmlen = dl_read_out (&r->cur.dl, r->temp_mem, r->lidx);
  ASSERT (r->tmlen <= r->tmcap);

  // 2) write main data
  werr_t (rpt_append_here (src, size * n, r));

  // 3) write tail back
  werr_t (rpt_append_here (r->temp_mem, r->tmlen, r));
  r->tmlen = 0;

  // 4) final commit
  werr_t (pgr_commit (r->pager, r->cur.dl.raw, r->cur.pg));

  return SUCCESS;
}

static err_t
rpt_read_next (u8 *dest, b_size *bytes, rptree *r)
{
  ASSERT (bytes);
  ASSERT (*bytes > 0);
  rptree_assert (r);

  b_size read = 0;
  while (read < *bytes)
    {
      if (*r->cur.dl.blen == r->lidx)
        {
          // No more pages left, return
          if (r->cur.pg == 0)
            {
              *bytes = read;
              return SUCCESS;
            }

          // Fetch the next page
          werr_t (pgr_get_expect (
              &r->cur,
              PG_DATA_LIST,
              r->cur.pg, r->pager));
          r->lidx = 0;
        }

      p_size _read = dl_read (
          &r->cur.dl,
          dest ? dest + read : NULL,
          r->lidx,
          *bytes - read);

      read += _read;
      r->lidx += _read;
    }

  ASSERT (read == *bytes);
  return SUCCESS;
}

err_t
rpt_read (u8 *dest, t_size size, b_size *n, b_size nskip, rptree *r)
{
  ASSERT (dest);
  ASSERT (size > 0);
  ASSERT (n);
  ASSERT (*n > 0);
  ASSERT (nskip >= 1);
  rptree_assert (r);

  b_size toread = size * *n;

  if (nskip == 1)
    {
      werr_t (rpt_read_next (dest, &toread, r));

      /**
       * Next should either == 0 or 1 * size
       */
      ASSERT (toread <= size * *n);
      if (toread % size != 0)
        {
          return ERR_INVALID_STATE;
        }

      ASSERT (toread % size == 0);
      *n = toread / size;
      return SUCCESS;
    }

  b_size read = 0;
  b_size next;

  while (read < toread)
    {
      next = size;
      werr_t (rpt_read_next (dest + read, &next, r));
      read += next;

      /**
       * Next should either == 0 or 1 * size
       */
      if (next == 0)
        {
          ASSERT (read % size == 0);
          *n = read / size;
          return SUCCESS;
        }
      else if (next != size)
        {
          // rpt is telling me it's at the end, but
          // it read a non multiple of size -
          // I think this logic might not belong here
          return ERR_INVALID_STATE;
        }

      // Skip values
      next = size * (nskip - 1);
      werr_t (rpt_read_next (NULL, &next, r));

      ASSERT (next <= size * (nskip - 1));
      if (next < size * (nskip - 1))
        {
          if (next % size != 0)
            {
              return ERR_INVALID_STATE;
            }

          // We reached the end
          ASSERT (read % size == 0);
          *n = read / size;
          return SUCCESS;
        }
    }

  return SUCCESS;
}
