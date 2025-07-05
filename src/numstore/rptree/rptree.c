#include "numstore/rptree/rptree.h"

#include "core/dev/assert.h"   // DEFINE_DBG_ASSERT_I
#include "core/errors/error.h" // TODO
#include "core/intf/io.h"      // TODO

#include "numstore/paging/page.h"            // TODO
#include "numstore/paging/pager.h"           // TODO
#include "numstore/paging/types/data_list.h" // dl_get_next
#include "numstore/rptree/internal/dld.h"    // TODO
#include "numstore/rptree/internal/dli.h"    // dli
#include "numstore/rptree/internal/dlr.h"    // TODO
#include "numstore/rptree/internal/dlw.h"    // TODO
#include "numstore/rptree/internal/ini.h"    // ini

static const char *TAG = "R+Tree";

/**
 * A "seek result" is the response to a seek call
 * on a rpt tree.
 *
 * It contains:
 * 1. A resulting Data List page - (where am I?)
 * 2. Where we are in the global database (gidx)
 * 3. Where we are in the local page (lidx)
 * 4. Stack (and length) of previous pages we visited to get here
 */
typedef struct
{
  pgno pg;     // The page number of the page we traversed
  p_size lidx; // Choice of the leaf we went with
} seek_v;      // rpt stack value

typedef struct
{
  pgno pg;          // The page we landed on
  seek_v stack[20]; // A stack of previous inner nodes and choices
  u32 sp;           // Stack pointer (also the length of the stack)
} seek_r;

DEFINE_DBG_ASSERT_I (seek_r, seek_r, r)
{
  ASSERT (r);
  ASSERT (r->sp <= 20);
}

struct rptree_s
{
  /**
   * Indexing (where am I?)
   */
  struct
  {
    b_size gidx; // The global byte I'm on
    b_size lidx; // The local byte (within the page I'm on)
    bool eof;
    pgno pg;
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

DEFINE_DBG_ASSERT_I (rptree, rptree, r)
{
  ASSERT (r);
  if (r->is_seeked)
    {
      seek_r_assert (&r->seek);
    }
}

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

  ret->seek = (seek_r){ 0 };
  ret->is_seeked = false;
  ret->pager = p;
  ret->pg0 = pg->pg;

  return ret;

failed:
  i_free (ret);
  return NULL;
}

void
rpt_close (rptree *r)
{
  rptree_assert (r);
  i_free (r);
}

/////////////////////////////////////////////////////////////////////
////////////// SECTION: rpt_seek

// Forward declaration
static err_t rpt_seek_recurs (
    rptree *r,
    b_size byte,
    const page *cur,
    error *e);

err_t
rpt_seek (rptree *r, b_size b, error *e)
{
  rptree_assert (r);

  const page *start = pgr_get (PG_DATA_LIST | PG_INNER_NODE, r->pg0, r->pager, e);
  if (start == NULL)
    {
      return err_t_from (e);
    }

  r->seek = (seek_r){
    // .stack
    .sp = 0,
  };

  r->gidx = 0;
  r->lidx = 0;
  r->eof = false;

  err_t_wrap (rpt_seek_recurs (r, b, start, e), e);

  r->is_seeked = true;

  return SUCCESS;
}

static err_t
rpt_seek_push_top (rptree *r, pgno pg, p_size lidx, error *e)
{
  rptree_assert (r);

  // Check for seek stack overflow
  if (r->seek.sp == 20)
    {
      return error_causef (
          e, ERR_PAGE_STACK_OVERFLOW,
          "Seek: Page stack overflow");
    }

  // Push element to the top of the stack
  r->seek.stack[r->seek.sp++] = (seek_v){
    .pg = pg,
    .lidx = lidx,
  };

  return SUCCESS;
}

err_t
rpt_seek_push_bottom (rptree *r, const page *p, p_size lidx, error *e)
{
  rptree_assert (r);

  if (r->seek.sp == 20)
    {
      return error_causef (
          e, ERR_PAGE_STACK_OVERFLOW,
          "Seek: Page stack overflow");
    }

  // Shift stack up 1 to make room
  for (u32 i = r->seek.sp++; i > 0; --i)
    {
      r->seek.stack[i] = r->seek.stack[i - 1];
    }

  /**
   * Push res to the bottom
   */
  r->seek.stack[0] = (seek_v){
    .pg = p->pg,
    .lidx = lidx,
  };

  return SUCCESS;
}

static inline err_t
rpt_seek_recurs_inner_node (
    rptree *r,
    b_size byte,
    const page *cur,
    error *e)
{
  rptree_assert (r);
  ASSERT (cur);

  // First, pick the index of the leaf we want to traverse
  r->lidx = in_choose_lidx (&cur->in, byte);

  // Push it onto the top of the stack
  err_t_wrap (rpt_seek_push_top (r, cur->pg, r->lidx, e), e);

  // Fetch the key and value at that location
  pgno next = in_get_leaf (&cur->in, r->lidx);
  b_size nleft = r->lidx > 0 ? in_get_key (&cur->in, r->lidx - 1) : 0;

  /**
   * We are "skipping" [nleft] bytes so add that
   * to gidx and subtract it from our next query index
   * for the layer down
   *
   * This is one of the key steps of a Rope
   */
  ASSERT (byte >= nleft);
  r->gidx += nleft;
  byte -= nleft;

  /**
   * Fetch the next page, which can either be
   * an inner node or a data list
   */
  cur = pgr_get (PG_INNER_NODE | PG_DATA_LIST, next, r->pager, e);

  if (cur == NULL)
    {
      return err_t_from (e);
    }

  return rpt_seek_recurs (r, byte, cur, e);
}

static inline void
rpt_seek_recurs_data_list (
    rptree *r,
    b_size byte,
    const page *cur)
{
  rptree_assert (r);
  ASSERT (cur);

  /**
   * Clip the byte to the maximum of this node
   * Otherwise, set it to the requested byte
   */
  if ((p_size)byte >= dl_used (&cur->dl))
    {
      /**
       * Proof that this case is only entered on the last
       * data list page:
       *
       *    0 < bytes < right key  (chose the far left key)
       *    or
       *    Left key < bytes < right key
       *    or
       *    left key < bytes < LENGTH (chose the far right key)
       *
       * TODO - finish this proof
       */
      r->lidx = dl_used (&cur->dl);
      r->eof = true;
    }
  else
    {
      r->lidx = byte;
    }

  // Update global idx and final result page
  r->gidx += r->lidx;
  r->seek.pg = cur->pg;
  r->pg = r->seek.pg;
}

err_t
rpt_seek_recurs (
    rptree *r,
    b_size byte,
    const page *cur,
    error *e)
{
  rptree_assert (r);
  switch (cur->type)
    {
    case PG_INNER_NODE:
      {
        return rpt_seek_recurs_inner_node (r, byte, cur, e);
      }
    case PG_DATA_LIST:
      {
        rpt_seek_recurs_data_list (r, byte, cur);
        return SUCCESS;
      }
    default:
      {
        // pgr_get_expect protects us from this branch
        UNREACHABLE ();
      }
    }
}

/////////////////////////////////////////////////////////////////////
////////////// Utils

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

/////////////////////////////////////////////////////////////////////
////////////// SECTION: rpt_read

static sb_size
rpt_read_contiguous (u8 *dest, b_size bytes, rptree *r, error *e)
{
  ASSERT (bytes);
  ASSERT (bytes > 0);
  ASSERT (r->is_seeked);
  ok_error_assert (e);
  rptree_assert (r);

  b_size read = 0;

  const page *cur = pgr_get (PG_DATA_LIST, r->pg, r->pager, e);
  if (cur == NULL)
    {
      bad_error_assert (e);
      return err_t_from (e);
    }

  while (read < bytes)
    {
      // Filled up
      if (dl_used (&cur->dl) == r->lidx)
        {
          pgno _next = dl_get_next (&cur->dl);

          // We are at the end
          if (_next == 0)
            {
              r->eof = true;
              return read;
            }

          // Fetch the next page
          const page *next = pgr_get (PG_DATA_LIST, _next, r->pager, e);
          if (next == NULL)
            {
              bad_error_assert (e);
              return err_t_from (e);
            }
          cur = next;
          r->pg = cur->pg;

          r->lidx = 0;
        }

      ASSERT (bytes > read);
      p_size _read = dl_read (
          &cur->dl,
          dest ? dest + read : NULL,
          r->lidx,
          bytes - read);

      read += _read;
      r->lidx += _read;
    }

  ASSERT (read == bytes);

  return read;
}

sb_size
rpt_read (
    u8 *dest,
    t_size size,
    b_size n,
    b_size stride,
    rptree *r,
    error *e)
{
  ASSERT (dest);
  ASSERT (size > 0);
  ASSERT (n > 0);
  ASSERT (stride >= 1);
  rptree_assert (r);

  // First, seek to the position we want
  if (!r->is_seeked)
    {
      err_t_wrap (rpt_seek (r, 0, e), e);
    }

  b_size btoread = size * n;

  // Make 1 contiguous read if no stride
  if (stride == 1)
    {
      sb_size nbytes = rpt_read_contiguous (dest, btoread, r, e);
      if (nbytes < 0)
        {
          bad_error_assert (e);
          return btoread;
        }

      ASSERT ((b_size)nbytes <= btoread);

      // Check if we could read a multiple of size bytes
      if (nbytes % size != 0)
        {
          return error_causef (
              e, ERR_CORRUPT,
              "%s: Premature EOF", TAG);
        }

      ASSERT (nbytes % size == 0);
      return nbytes / size;
    }

  // Otherwise do expected routine of skips
  b_size bread = 0;
  sb_size next;

  while (bread < btoread)
    {
      // Read [size] bytes
      next = rpt_read_contiguous (dest + bread, size, r, e);
      if (next < 0)
        {
          bad_error_assert (e);
          return err_t_from (e);
        }
      bread += next;

      if (next == 0)
        {
          ASSERT (bread % size == 0);
          return bread / size;
        }
      else if (next != size)
        {
          return error_causef (
              e, ERR_CORRUPT,
              "%s: Premature EOF while reading", TAG);
        }

      ASSERT (next > 0);

      // Skip (size * (stride - 1)) bytes
      next = rpt_read_contiguous (NULL, size * (stride - 1), r, e);
      if (next < 0)
        {
          bad_error_assert (e);
          return err_t_from (e);
        }
      // We read a non multiple of size - format error
      if (next % size != 0)
        {
          return error_causef (
              e, ERR_CORRUPT,
              "%s Premature EOF while reading", TAG);
        }

      if (rpt_eof (r))
        {
          ASSERT (bread % size == 0);
          return bread / size;
        }
    }

  ASSERT (bread == btoread);
  return bread / size;
}

/////////////////////////////////////////////////////////////////////
////////////// SECTION: rpt_delete

sb_size
rpt_delete (
    u8 *dest,
    t_size size,
    b_size n,
    b_size stride,
    rptree *r,
    error *e)
{
  (void)dest;
  rptree_assert (r);

  dld_params params = {
    .dest = NULL,
    .size = size,
    .n = n,
    .nskip = stride,
    .idx0 = r->lidx,
    .start = NULL, // NULL,
    .pager = r->pager,
  };

  return _rpt_dld (params, e);
}

sb_size
rpt_take (
    u8 *dest,
    t_size size,
    b_size n,
    b_size stride,
    rptree *r,
    error *e)
{
  rptree_assert (r);

  dld_params params = {
    .dest = dest,
    .size = size,
    .n = n,
    .nskip = stride,
    .idx0 = r->lidx,
    .start = NULL,
    .pager = r->pager,
  };

  return _rpt_dld (params, e);
}

/////////////////////////////////////////////////////////////////////
////////////// SECTION: rpt_insert

sb_size
rpt_insert (const u8 *src, b_size n, rptree *r, error *e)
{
  ASSERT (src);
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
    .start = r->seek.pg,
    .pager = r->pager,
    .src = src,
    .n = n,
    .fill_factor = 1,
  };

  // This is the return value because it's the bottom layer (e.g. the data)
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
      const page *cur_r = pgr_get (PG_INNER_NODE, next.pg, r->pager, e);

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
          err_t_wrap (rpt_seek_push_bottom (r, cur, 0, e), e);

          // Update the root node
          r->pg0 = cur->pg;

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

/////////////////////////////////////////////////////////////////////
////////////// SECTION: rpt_write

static sb_size
rpt_write_contiguous (
    const u8 *src,
    b_size bytes,
    rptree *r,
    error *e)
{
  ASSERT (bytes);
  ASSERT (bytes > 0);
  ASSERT (r->is_seeked);
  ok_error_assert (e);
  rptree_assert (r);

  b_size written = 0;

  const page *cur = pgr_get (PG_DATA_LIST, r->pg, r->pager, e);
  if (cur == NULL)
    {
      bad_error_assert (e);
      return err_t_from (e);
    }

  while (written < bytes)
    {
      // Filled up
      if (dl_used (&cur->dl) == r->lidx)
        {
          pgno _next = dl_get_next (&cur->dl);

          // We are at the end
          if (_next == 0)
            {
              r->eof = true;
              return written;
            }

          // Fetch the next page
          const page *next = pgr_get (PG_DATA_LIST, _next, r->pager, e);
          if (next == NULL)
            {
              bad_error_assert (e);
              return err_t_from (e);
            }
          cur = next;
          r->pg = cur->pg;

          r->lidx = 0;
        }

      ASSERT (bytes > written);

      page *_cur = pgr_make_writable (r->pager, cur);
      ASSERT (cur != NULL); // Temporary until we throw for make_writable

      p_size _write = dl_write (
          &_cur->dl,
          src ? src + written : NULL,
          r->lidx,
          bytes - written);

      written += _write;
      r->lidx += _write;
    }

  ASSERT (written == bytes);

  return written;
}

sb_size
rpt_write (
    const u8 *src,
    t_size size,
    b_size n,
    b_size stride,
    rptree *r,
    error *e)
{
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n > 0);
  ASSERT (stride >= 1);
  rptree_assert (r);

  // First, seek to the position we want
  if (!r->is_seeked)
    {
      err_t_wrap (rpt_seek (r, 0, e), e);
    }

  b_size btowrite = size * n;

  // Make 1 contiguous write if no stride
  if (stride == 1)
    {
      sb_size nbytes = rpt_write_contiguous (src, btowrite, r, e);
      if (nbytes < 0)
        {
          bad_error_assert (e);
          return btowrite;
        }

      ASSERT ((b_size)nbytes <= btowrite);

      // Check if we could write a multiple of size bytes
      if (nbytes % size != 0)
        {
          return error_causef (
              e, ERR_CORRUPT,
              "%s: Premature EOF", TAG);
        }

      ASSERT (nbytes % size == 0);
      return nbytes / size;
    }

  // Otherwise do expected routine of skips
  b_size bwrite = 0;
  sb_size next;

  while (bwrite < btowrite)
    {
      // Read [size] bytes
      next = rpt_write_contiguous (src + bwrite, size, r, e);
      if (next < 0)
        {
          bad_error_assert (e);
          return err_t_from (e);
        }
      bwrite += next;

      if (next == 0)
        {
          ASSERT (bwrite % size == 0);
          return bwrite / size;
        }
      else if (next != size)
        {
          return error_causef (
              e, ERR_CORRUPT,
              "%s: Premature EOF while writing", TAG);
        }

      ASSERT (next > 0);

      // Skip (size * (stride - 1)) bytes
      next = rpt_write_contiguous (NULL, size * (stride - 1), r, e);
      if (next < 0)
        {
          bad_error_assert (e);
          return err_t_from (e);
        }
      // We write a non multiple of size - format error
      if (next % size != 0)
        {
          return error_causef (
              e, ERR_CORRUPT,
              "%s Premature EOF while writing", TAG);
        }

      if (rpt_eof (r))
        {
          ASSERT (bwrite % size == 0);
          return bwrite / size;
        }
    }

  ASSERT (bwrite == btowrite);
  return bwrite / size;
}
