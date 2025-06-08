#include "rptree/rptree.h"

#include "dev/assert.h" // DEFINE_DBG_ASSERT_I
#include "errors/error.h"
#include "intf/io.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "paging/types/data_list.h" // dl_get_next

#include "rptree/internal/dld.h"
#include "rptree/internal/dli.h" // dli
#include "rptree/internal/dlr.h"
#include "rptree/internal/dlw.h"
#include "rptree/internal/ini.h" // ini
#include "rptree/internal/seek.h"

DEFINE_DBG_ASSERT_I (rptree, rptree, r)
{
  ASSERT (r);
}

static const char *TAG = "R+Tree";

struct rptree_s
{
  /**
   * Indexing (where am I?)
   */
  struct
  {
    b_size gidx;  // The global byte I'm on
    b_size lidx;  // The local byte (within the page I'm on)
    bool writing; // If true, _rw else _r
    union
    {
      const page *cur_r; // Current page we're on
      page *cur_rw;      // Writable
    };
    bool eof;
  };

  /**
   * Seek result (a stack)
   */
  struct
  {
    seek_r seek; // Result from a seek call
    bool is_seeked;
  };

  pager *pager;
  pgno pg0;
};

//////////////////////////////// LIFECYCLE

rptree *
rpt_open (spgno pg0, pager *p, error *e)
{
  rptree *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "Failed to allocate rptree");
      return NULL;
    }

  ret->gidx = 0;
  ret->lidx = 0;

  const page *pg;

  if (pg0 < 0)
    {
      // Create a new page
      pg = pgr_new (p, PG_DATA_LIST, e);
    }
  else
    {
      // Get existing page
      pg = pgr_get (PG_DATA_LIST | PG_INNER_NODE, pg0, p, e);
    }

  if (pg == NULL)
    {
      goto failed;
    }

  ret->writing = false;
  ret->cur_r = pg;
  ret->seek = (seek_r){ 0 };
  ret->is_seeked = false;
  ret->pager = p;
  ret->pg0 = pg->pg;

  return ret;

failed:
  i_free (ret);
  return NULL;
}

err_t
rpt_seek (rptree *r, b_size b, error *e)
{
  rptree_assert (r);

  seek_params params = {
    .starting_page = r->cur_r,
    .pager = r->pager,
    .whereto = b,
    .scap = 10,
  };

  err_t_wrap (_rpt_seek (&r->seek, params, e), e);

  r->is_seeked = true;

  return SUCCESS;
}

b_size
rpt_tell (rptree *r)
{
  rptree_assert (r);
  return r->gidx;
}

bool
rpt_eof (rptree *r)
{
  rptree_assert (r);
  return r->eof;
}

pgno
rpt_pg0 (rptree *r)
{
  rptree_assert (r);
  return r->pg0;
}

static sb_size
rpt_read_contiguous (u8 *dest, b_size bytes, rptree *r, error *e)
{
  ASSERT (bytes);
  ASSERT (bytes > 0);
  rptree_assert (r);

  b_size read = 0;

  while (read < bytes)
    {
      if (dl_used (&r->cur_r->dl) == r->lidx)
        {
          pgno _next = dl_get_next (&r->cur_r->dl);

          // We are at the end
          if (_next == 0)
            {
              return read;
            }

          // Fetch the next page
          const page *next = pgr_get (PG_DATA_LIST, _next, r->pager, e);
          if (next == NULL)
            {
              return err_t_from (e);
            }
          r->cur_r = next;

          r->lidx = 0;
        }

      p_size _read = dl_read (
          &r->cur_r->dl,
          dest ? dest + read : NULL,
          r->lidx,
          bytes - read);

      read += _read;
      r->lidx += _read;
    }

  ASSERT (read == bytes);

  return SUCCESS;
}

sb_size
rpt_read (
    u8 *dest, t_size size, b_size n, b_size nskip,
    rptree *r, error *e)
{
  ASSERT (dest);
  ASSERT (size > 0);
  ASSERT (n > 0);
  ASSERT (nskip >= 1);
  rptree_assert (r);

  b_size toread = size * n;

  if (nskip == 1)
    {
      sb_size nbytes = rpt_read_contiguous (dest, toread, r, e);

      if (nbytes < 0)
        {
          return toread;
        }

      /**
       * Next should either == 0 or 1 * size
       */
      ASSERT ((b_size)nbytes <= size * n);
      if (nbytes % size != 0)
        {
          return error_causef (
              e, ERR_CORRUPT,
              "%s: Premature EOF", TAG);
        }

      ASSERT (nbytes % size == 0);
      return nbytes / size;
    }

  b_size read = 0;
  sb_size next;

  while (read < toread)
    {
      next = rpt_read_contiguous (dest + read, size, r, e);

      read += next;

      /**
       * Next should either == 0 or 1 * size
       */
      if (next == 0)
        {
          ASSERT (read % size == 0);
          return read / size;
        }
      else if (next != size)
        {
          return error_causef (
              e, ERR_CORRUPT,
              "%s: Premature EOF", TAG);
        }

      // Skip values
      next = rpt_read_contiguous (NULL, size * (nskip - 1), r, e);
      if (next < 0)
        {
          return err_t_from (e);
        }

      ASSERT ((b_size)next <= size * (nskip - 1));
      if ((b_size)next < size * (nskip - 1))
        {
          if (next % size != 0)
            {
              // TODO - better error message
              return error_causef (
                  e, ERR_CORRUPT,
                  "%s Premature EOF", TAG);
            }

          // We've reached the end
          ASSERT (read % size == 0);
          return read / size;
          return SUCCESS;
        }
    }

  return SUCCESS;
}

sb_size
rpt_delete (
    u8 *dest, t_size size, b_size n, b_size nskip,
    rptree *r, error *e)
{
  (void)dest;
  rptree_assert (r);

  dld_params params = {
    .dest = NULL,
    .size = size,
    .n = n,
    .nskip = nskip,
    .idx0 = r->lidx,
    .start = r->cur_r,
    .pager = r->pager,
  };

  return _rpt_dld (params, e);
}

sb_size
rpt_take (
    u8 *dest, t_size size, b_size n, b_size nskip,
    rptree *r, error *e)
{
  rptree_assert (r);

  dld_params params = {
    .dest = dest,
    .size = size,
    .n = n,
    .nskip = nskip,
    .idx0 = r->lidx,
    .start = r->cur_r,
    .pager = r->pager,
  };

  return _rpt_dld (params, e);
}

sb_size
rpt_insert (
    const u8 *src, t_size size, b_size n,
    rptree *r, error *e)
{
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n > 0);
  rptree_assert (r);

  if (!r->is_seeked)
    {
      err_t_wrap (rpt_seek (r, 0, e), e);
    }

  /**
   * Run [dli]
   *
   * At the bottom layer (data list nodes) create
   * new data list nodes and fill them with [src]
   */
  mem_inner_node out;

  dli_params params = {
    .idx0 = r->lidx,
    .start = r->cur_r,
    .pager = r->pager,
    .src = src,
    .size = size,
    .n = n,
    .fill_factor = 1,
  };

  // This is the return value because it's the bottom layer
  sb_size written = _rpt_dli (&out, params, e);
  if (written < 0)
    {
      // TODO - rollback
      return err_t_from (e);
    }

  /**
   * Traverse from the root down to current location and update
   * the keys of each inner node to the right of the chosen one
   * with new bytes with the number of new data values we wrote
   */
  for (u32 i = 0; i < r->seek.sp; ++i)
    {
      // The next layer to update - from top to bottom
      seek_v next = r->seek.stack[i];

      // Fetch that page
      const page *cur_r = pgr_get (
          PG_INNER_NODE,
          next.pg,
          r->pager,
          e);

      if (cur_r == NULL)
        {
          return err_t_from (e);
        }

      // TODO - Optimization - on far right nodes, you don't need
      // to write them
      // Get it working first, though
      page *cur = pgr_make_writable (r->pager, cur_r);

      // Add [btotal] to the right
      in_add_right (&cur->in, next.lidx, written);
    }

  /**
   * Step 3 - While we still have overflow, move up the stack
   * to create new inner nodes and split them
   */
  int sp = r->seek.sp;        // Top of the stack
  mem_inner_node input = out; // Will swap on each loop

  // While there are more overflow nodes in [input]
  // loop through and write up the layers
  if (input.klen > 0)
    {
      const page *cur;
      p_size from;

      if (sp == 0)
        {
          /**
           * We are at the base of the stack and we
           * need more room, create a new root node and push it
           * onto the stack
           */
          cur = pgr_new (r->pager, PG_INNER_NODE, e);
          if (cur == NULL)
            {
              return err_t_from (e);
            }

          // Push this new node to the bottom of the stack
          err_t_wrap (_rpt_seek_r_push_bottom (&r->seek, cur, 0, e), e);

          // New page size has 0 size so
          // start at 0 key
          from = 0;
        }
      else
        {
          /**
           * We are at an inner node, fetch that node
           */
          seek_v v = r->seek.stack[--sp];
          cur = pgr_get (PG_INNER_NODE, v.pg, r->pager, e);
          if (cur == NULL)
            {
              return err_t_from (e);
            }

          from = v.lidx;
        }

      /**
       * Run [ini]
       * At this inner node - write a bunch of inner nodes to
       * the right until input is empty
       */
      ini_params iparams = {
        .idx0 = from,
        .start = cur,
        .pager = r->pager,
        .input = input,
        .fill_factor = 0.5,
      };
      err_t_wrap (_rpt_ini (&out, iparams, e), e);
    }

  return written;
}

sb_size
rpt_write (
    const u8 *src, t_size size, b_size n, b_size nskip,
    rptree *r, error *e)
{
  rptree_assert (r);

  dlw_params params = {
    .idx0 = r->lidx,
    .start = r->cur_r,
    .pager = r->pager,
    .src = src,
    .size = size,
    .n = n,
    .nskip = nskip,
  };

  return _rpt_dlw (params, e);
}
