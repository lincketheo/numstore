#include "rptree/rptree.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "paging/types/data_list.h"
#include "paging/types/inner_node.h"
#include "rptree/dliacin.h"
#include "rptree/iniacin.h"
#include "rptree/mem_inner_node.h"
#include "rptree/seek.h"

DEFINE_DBG_ASSERT_I (rptree, rptree, r)
{
  ASSERT (r);
  if (!r->is_open)
    {
      ASSERT (!r->is_seeked);
    }
}

//////////////////////////////// LIFECYCLE

err_t
rpt_create (rptree *r, rpt_params p)
{
  /**
   * Allocate resources.
   *
   * TODO - make this less memory hungry
   */
  u8 *temp_mem = lmalloc (p.alloc, p.page_size * sizeof *temp_mem);

  err_t seek = rpts_create (
      &r->seek,
      (rpts_params){
          .alloc = p.alloc,
          .scap = 10,
      });

  if (temp_mem == NULL || seek)
    {
      goto failed;
    }

  *r = (rptree){
    .gidx = 0,          // meaningless if !is_open
    .lidx = 0,          // ^^
    .cur = (page){ 0 }, // ^^
    .is_open = false,

    // r->seek
    .is_seeked = false,

    .temp_mem = temp_mem,
    .tmlen = 0,
    .tmcap = p.page_size,
    .alloc = p.alloc,

    .pager = p.pager,
  };

  return SUCCESS;

failed:
  if (temp_mem)
    {
      lfree (p.alloc, temp_mem);
    }
  if (seek == SUCCESS)
    {
      rpts_free (&r->seek);
    }

  return ERR_NOMEM;
}

err_t
rpt_new (rptree *r)
{
  rptree_assert (r);
  ASSERT (!r->is_open);

  err_t ret;

  /**
   * Create a new page
   */
  if ((ret = pgr_new (&r->cur, r->pager, PG_DATA_LIST)))
    {
      return ret;
    }

  r->is_open = true;

  return SUCCESS;
}

err_t
rpt_open (rptree *r, pgno p0)
{
  rptree_assert (r);
  ASSERT (!r->is_open);

  err_t ret;

  // Fetch the root page
  if ((ret = pgr_get_expect (
           &r->cur,
           PG_INNER_NODE | PG_DATA_LIST,
           p0, r->pager)))
    {
      return ret;
    }

  r->is_open = true;

  return SUCCESS;
}

void
rpt_close (rptree *r)
{
  rptree_assert (r);
  ASSERT (r->is_open);
  r->is_open = false;
  r->is_seeked = false;
  rpts_reset (&r->seek);
}

static err_t
rpt_seek_recursive (rptree *r, b_size byte)
{
  rptree_assert (r);

  err_t ret = SUCCESS;

  switch (r->cur.type)
    {
    case PG_INNER_NODE:
      {
        // Choose which leaf to traverse to next
        p_size lidx = in_choose_lidx (&r->cur.in, byte);

        // Push it to the top of the stack
        if ((ret = rpts_push_page (&r->seek, r->cur, lidx)))
          {
            // Not sure what to do yet on failure
            panic ();
          }

        // Fetch key and value at that location
        pgno next = r->cur.in.leafs[lidx];
        b_size nleft = 0;
        if (lidx > 0)
          {
            nleft = r->cur.in.keys[lidx - 1];
          }

        // Subtract left quantity from byte query
        ASSERT (byte >= nleft);
        r->gidx += nleft;
        byte -= nleft;

        // Fetch the next page
        if ((ret = pgr_get_expect (
                 &r->cur,
                 PG_INNER_NODE | PG_DATA_LIST,
                 next, r->pager)))
          {
            panic ();
          }

        // Recursive call
        return rpt_seek_recursive (r, byte);
      }
    case PG_DATA_LIST:
      {
        if ((p_size)byte >= *r->cur.dl.blen)
          {
            // Clip byte to the last byte of this node
            r->lidx = *r->cur.dl.blen;
          }
        else
          {
            // Set local id to byte
            r->lidx = byte;
          }

        // Push it to the top of the stack
        if ((ret = rpts_push_page (&r->seek, r->cur, r->lidx)))
          {
            // Not sure what to do yet on failure
            panic ();
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
rpt_seek (rptree *r, b_size b)
{
  rptree_assert (r);
  ASSERT (r->is_open);
  ASSERT (!r->is_seeked);

  /**
   * TODO - this doesn't need to be recursive
   */
  return rpt_seek_recursive (r, b);
}

err_t
rpt_insert (const u8 *src, t_size size, b_size n, rptree *r)
{
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n > 0);
  rptree_assert (r);
  ASSERT (r->is_seeked);

  err_t ret = SUCCESS;

  /**
   * Step 1
   * Traverse from the root down to current location and update
   * the keys of each inner node to the right of the chosen one
   * with new bytes
   */
  b_size btotal = n * size;
  for (u32 i = 0; i < rpts_len (&r->seek); ++i)
    {
      rpts_v s = rpts_get (&r->seek, i);
      in_add_right (&s.page.in, s.lidx, btotal);
    }

  /**
   * Step 2 Create the first overflow buffer by writing out last
   * data node
   */
  mem_inner_node out;
  if ((ret = dliacin (
           &out,
           (dliacin_params){
               .idx0 = r->lidx,
               .pg0 = r->cur,
               .pager = r->pager,
               .alloc = r->alloc,
               .src = src,
               .size = size,
               .n = n,
           })))
    {
      return ret;
    }

  /**
   * Next,
   * Work up the page stack and feed mem_inner_node into higher level
   * inner nodes. If none exist, push new roots
   */
  ASSERT (rpts_len (&r->seek) > 0);
  u32 sp = rpts_len (&r->seek) - 1; // Start at the top - work down

  mem_inner_node in = out;
  while (in.kvlen > 0)
    {
      page cur;    // The next page we want to update
      p_size from; // The key index inside cur we want to append to

      if (sp == 0)
        {
          if (pgr_new (&cur, r->pager, PG_INNER_NODE))
            {
              return ret;
            }

          if (rpts_push_to_bottom (&r->seek, cur, 0))
            {
              return ret;
            }
          from = 0; // Start from the beginning
        }
      else
        {
          rpts_v v = r->seek.stack[--sp];
          cur = v.page;
          from = v.lidx;
        }

      mem_inner_node out;
      ret = iniacin (
          &out, (iniacin_params){
                    .input = in,
                    .idx0 = from,
                    .alloc = r->alloc,
                    .pager = r->pager,
                    .pg0 = cur,
                });
      if (ret)
        {
          return ret;
        }
      in = out;
    }

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
          if (*r->cur.dl.next == 0)
            {
              *bytes = read;
              return SUCCESS;
            }

          // Fetch the next page
          werr_t (pgr_get_expect (
              &r->cur,
              PG_DATA_LIST,
              *r->cur.dl.next,
              r->pager));
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
