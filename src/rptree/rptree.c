#include "rptree/rptree.h"

#include "dev/assert.h"
#include "errors/error.h"
#include "intf/stdlib.h"
#include "mm/lalloc.h"
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
rpt_create (rptree *dest, rpt_params p, error *e)
{
  rptree ret = {
    .gidx = 0,          // meaningless if !is_open
    .lidx = 0,          // ^^
    .cur = (page){ 0 }, // ^^
    .is_open = false,

    // r->seek // Nothing
    .is_seeked = false,

    .pager = p.pager,
  };

  u32 space = p.pager->page_size + 2048;
  err_t_wrap (lalloc_reserve (&ret.espace_alloc, p.alloc, space, e), e);
  *dest = ret;

  rptree_assert (&ret);

  return SUCCESS;
}

err_t
rpt_new (pgno *dest, rptree *r, error *e)
{
  rptree_assert (r);
  ASSERT (!r->is_open);

  err_t_wrap (pgr_new (&r->cur, r->pager, PG_DATA_LIST, e), e);

  *dest = r->cur.pg;
  r->is_open = true;

  return SUCCESS;
}

err_t
rpt_open (rptree *r, pgno p0, error *e)
{
  rptree_assert (r);
  ASSERT (!r->is_open);

  // Fetch the root page
  err_t_wrap (
      pgr_get_expect (
          &r->cur,
          PG_INNER_NODE | PG_DATA_LIST,
          p0, r->pager, e),
      e);

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
  // rpts_reset (&r->seek);
}

err_t
rpt_seek (rptree *r, b_size b, error *e)
{
  rptree_assert (r);

  seek_params params = {
    .starting_page = r->cur,
    .pager = r->pager,
    .whereto = b,
    .scap = 10,
  };

  err_t_wrap (seek (&r->seek, params, e), e);

  r->is_seeked = true;

  return SUCCESS;
}

err_t
rpt_insert (const u8 *src, t_size size, b_size n, rptree *r, error *e)
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
  for (u32 i = 0; i < r->seek.sp; ++i)
    {
      page cur;
      seek_v next = r->seek.stack[i];
      err_t_wrap (pgr_get_expect (&cur, PG_INNER_NODE, next.pg, r->pager, e), e);
      {
        return ret;
      }
      in_add_right (&cur.in, next.lidx, btotal);
    }

  /**
   * Step 2 Create the first overflow buffer by writing out last
   * data node
   */
  mem_inner_node out;

  dliacin_params params = {
    .idx0 = r->lidx,
    .pg0 = r->cur,
    .pager = r->pager,
    .alloc = &r->espace_alloc,
    .src = src,
    .size = size,
    .n = n,
  };
  err_t_wrap (dliacin (&out, params, e), e);

  int sp = r->seek.sp;
  mem_inner_node input = out;

  if (input.kvlen > 0)
    {
      page cur;
      p_size from;

      if (sp == 0)
        {
          /**
           * We are at the base of the stack and we
           * need more room, create a new root node and push it
           * onto the stack
           */
          err_t_wrap (pgr_new (&cur, r->pager, PG_INNER_NODE, e), e);

          err_t_wrap (seek_r_push_to_bottom (&r->seek, cur, 0, e), e);

          from = 0;
        }
      else
        {
          /**
           * We are at an inner node, fetch that node
           */
          seek_v v = r->seek.stack[--sp];
          err_t_wrap (pgr_get_expect (&cur, PG_INNER_NODE, v.pg, r->pager, e), e);

          from = v.lidx;
        }

      iniacin_params iparams = {
        .input = input,
        .idx0 = from,
        .alloc = &r->espace_alloc,
        .pager = r->pager,
        .pg0 = cur,
      };
      err_t_wrap (iniacin (&out, iparams, e), e);
    }
  return ret;
}

static err_t
rpt_read_next (u8 *dest, b_size *bytes, rptree *r, error *e)
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
          err_t_wrap (pgr_get_expect (&r->cur, PG_DATA_LIST, *r->cur.dl.next, r->pager, e), e);

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
rpt_read (u8 *dest, t_size size, b_size *n, b_size nskip, rptree *r, error *e)
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
      err_t_wrap (rpt_read_next (dest, &toread, r, e), e);

      /**
       * Next should either == 0 or 1 * size
       */
      ASSERT (toread <= size * *n);
      if (toread % size != 0)
        {
          return error_causef (
              e, ERR_CORRUPT,
              "Expecting data list to have data in multiples "
              "of %" PRt_size " but read: %" PRb_size " bytes",
              size, toread);
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
      err_t_wrap (rpt_read_next (dest + read, &next, r, e), e);

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
          // TODO - better error message
          return error_causef (
              e, ERR_CORRUPT,
              "Failed to read a multiple of %d bytes", size);
        }

      // Skip values
      next = size * (nskip - 1);
      err_t_wrap (rpt_read_next (NULL, &next, r, e), e);

      ASSERT (next <= size * (nskip - 1));
      if (next < size * (nskip - 1))
        {
          if (next % size != 0)
            {
              // TODO - better error message
              return error_causef (
                  e, ERR_CORRUPT,
                  "Failed to read a multiple of %d bytes", size);
            }

          // We reached the end
          ASSERT (read % size == 0);
          *n = read / size;
          return SUCCESS;
        }
    }

  return SUCCESS;
}
