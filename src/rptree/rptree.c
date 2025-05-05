#include "rptree/rptree.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "paging/types/data_list.h"
#include "paging/types/inner_node.h"

DEFINE_DBG_ASSERT_I (overflow, overflow, o)
{
  ASSERT (o);
  ASSERT (o->keys);
  ASSERT (o->values);
  ASSERT (o->klen <= o->kcap);
  ASSERT (o->kcap > 0);
}

DEFINE_DBG_ASSERT_I (rptree, rptree, r)
{
  ASSERT (r);
  if (!r->is_open)
    {
      ASSERT (!r->is_seeked);
    }
  overflow_assert (&r->ofr);
  overflow_assert (&r->ofr);
}

//////////////////////////////// LIFECYCLE

static inline err_t
overflow_alloc (overflow *dest, p_size kcap, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->keys == NULL);
  ASSERT (dest->values == NULL);

  b_size *keys = lmalloc (alloc, kcap * sizeof *dest->keys);
  pgno *values = lmalloc (alloc, (kcap + 1) * sizeof *dest->values);

  if (!keys || !values)
    {
      if (keys)
        {
          lfree (alloc, keys);
        }
      if (values)
        {
          lfree (alloc, values);
        }
      return ERR_NOMEM;
    }

  dest->keys = keys;
  dest->values = values;
  dest->kcap = kcap;
  dest->klen = 0;

  overflow_assert (dest);

  return SUCCESS;
}

static inline void
overflow_free (overflow *dest, lalloc *alloc)
{
  overflow_assert (dest);
  ASSERT (dest);
  lfree (alloc, dest->keys);
  lfree (alloc, dest->values);
}

static inline err_t
rpt_alloc (rptree *r, lalloc *alloc, p_size page_size)
{
  u8 *temp_mem = lmalloc (alloc, page_size * sizeof *temp_mem);
  err_t ofr = overflow_alloc (&r->ofr, 10, alloc);
  err_t ofw = overflow_alloc (&r->ofw, 10, alloc);

  if (temp_mem == NULL || ofr || ofw)
    {
      goto failed;
    }

  r->temp_mem = temp_mem;
  r->tmlen = 0;
  r->tmcap = page_size;

  return SUCCESS;

failed:
  if (temp_mem)
    {
      lfree (alloc, temp_mem);
    }
  if (!ofr)
    {
      overflow_free (&r->ofr, alloc);
    }
  if (!ofw)
    {
      overflow_free (&r->ofw, alloc);
    }

  return ERR_NOMEM;
}

err_t
rpt_create (rptree *dest, rpt_params params)
{
  ASSERT (dest);

  // Allocate
  if (rpt_alloc (dest, params.alloc, params.page_size))
    {
      return ERR_NOMEM;
    }

  *dest = (rptree){
    // Current state
    .is_open = false,
    .is_seeked = false,

    .sp = 0,

    .alloc = params.alloc,
    .pager = params.pager,
  };

  return SUCCESS;
}

err_t
rpt_new (rptree *r, pgno *p0)
{
  rptree_assert (r);
  ASSERT (!r->is_open);
  ASSERT (p0);

  page top;
  werr_t (pgr_new (&top, r->pager, PG_DATA_LIST));

  return rpt_open (r, top.pg);
}

static inline void
rpt_push_page (rptree *r, page p, p_size lidx)
{
  rptree_assert (r);
  ASSERT (r->sp < 20);
  r->stack[r->sp++] = (rpt_stack_v){
    .page = p,
    .lidx = lidx,
  };
}

err_t
rpt_open (rptree *r, pgno p0)
{
  rptree_assert (r);
  ASSERT (!r->is_open);

  page top;

  // Fetch the root page
  werr_t (pgr_get_expect (
      &top,
      PG_INNER_NODE | PG_DATA_LIST,
      p0, r->pager));

  r->is_open = true;
  ASSERT (!r->is_seeked);
  ASSERT (r->sp == 0);

  r->cur = top;

  return SUCCESS;
}

void
rpt_close (rptree *r)
{
  rptree_assert (r);
  ASSERT (r->is_open);
  r->is_open = false;
  r->is_seeked = false;
  r->sp = 0;
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
        p_size lidx = in_choose_lidx (&r->cur.in, byte);

        // Push it to the top of the stack
        rpt_push_page (r, r->cur, lidx);

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
            // Clip byte to the last byte of this node
            r->lidx = *r->cur.dl.blen;
          }
        else
          {
            // Set local id to byte
            r->lidx = byte;
          }

        rpt_push_page (r, r->cur, r->lidx);

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
  return rpt_seek_recursive (r, b);
}

static inline err_t
overflow_push (overflow *ov, pgno pg, b_size key, lalloc *alloc)
{
  overflow_assert (ov);
  if (ov->klen == ov->kcap)
    {
      b_size *keys = lrealloc (alloc, ov->keys, 2 * ov->kcap * sizeof *keys);
      pgno *values = lrealloc (alloc, ov->values, (2 * ov->kcap + 1) * sizeof *values);
      if (!keys || !values)
        {
          panic ();
        }
      ov->kcap *= 2;
      ov->keys = keys;
      ov->values = values;
    }
  ov->keys[ov->klen] = key;
  ov->values[ov->klen++] = pg;
  return SUCCESS;
}

static inline void
overflow_push_last_page (overflow *ov, pgno pg)
{
  overflow_assert (ov);
  ov->values[ov->klen] = pg;
}

err_t
rpt_insert (const u8 *src, t_size size, b_size n, rptree *r)
{
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n > 0);
  rptree_assert (r);
  ASSERT (r->is_seeked);

  /**
   * Step 1
   * Traverse from the root down to current location and update
   * the keys of each inner node to the right of the chosen one
   * with new bytes
   */
  b_size btotal = n * size;
  ASSERT (r->sp > 0); // At least a data list node
  for (u32 i = 0; i < r->sp - 1; ++i)
    {
      rpt_stack_v s = r->stack[i];
      in_add_right (&s.page.in, s.lidx, btotal);
    }

  /**
   * Step 2
   * Save the right half of the current bottom data list node to
   * write at the end of everything
   */
  ASSERT (r->cur.type == PG_DATA_LIST);
  r->tmlen = dl_read_out_from (&r->cur.dl, r->temp_mem, r->lidx);
  ASSERT (r->tmlen < r->tmcap); // lt because no node is ever page_size

  /**
   * Step 3
   * Allocate room and create all bottom layer data nodes
   * and link them up correctly
   */
  b_size written = 0;
  page cur = r->cur;
  while (written < btotal)
    {
      /**
       * Step 1, check if the current node is full.
       * If so, allocate a new node and link and swap to it
       * Also push to the overflow buffer
       */
      if (dl_avail (&cur.dl) == 0)
        {
          // Allocate a new node
          page next;
          if (pgr_new (&next, r->pager, PG_DATA_LIST))
            {
              panic ();
            }

          // Link current node to it
          *cur.dl.next = next.pg;

          // Append this page and right key to overflow
          if (overflow_push (&r->ofw, cur.pg, *cur.dl.blen, r->alloc))
            {
              panic ();
            }

          // Commit so that we can swap it
          if (pgr_commit (r->pager, cur.dl.raw, cur.pg))
            {
              panic ();
            }

          // Swap it
          cur = next;
        }

      /**
       * Step 2
       * Write as many elements to this node as possible
       */
      p_size next_write = btotal - written;
      p_size just_wrote = dl_write (&cur.dl, src + written, next_write);

      written += just_wrote;
    }

  /**
   * Step 3,
   * What's left is inside tmp_mem. Try to write it, if there's more
   * allocate a new page and write the rest.
   * Garunteed to only need 1 more page because temp_mem used to fit
   * on 1 page
   */
  written = dl_write (&cur.dl, r->temp_mem, r->tmlen);
  if (written < r->tmlen)
    {
      // Allocate a new node
      page next;
      if (pgr_new (&next, r->pager, PG_DATA_LIST))
        {
          panic ();
        }

      // Link current node to it
      *cur.dl.next = next.pg;

      // Append this page and right key to overflow
      if (overflow_push (&r->ofw, cur.pg, *cur.dl.blen, r->alloc))
        {
          panic ();
        }

      // Commit so that we can swap it
      if (pgr_commit (r->pager, cur.dl.raw, cur.pg))
        {
          panic ();
        }

      // Swap it
      cur = next;

      p_size next_write = r->tmlen - written;
      p_size just_wrote = dl_write (&cur.dl, r->temp_mem + written, next_write);
      ASSERT (just_wrote == next_write);
    }

  /**
   * Step 4.
   * Append the last page to the overflow.
   * If klen = 0, then that's ok, it means we didn't actually
   * allocate any new nodes
   */
  overflow_push_last_page (&r->ofw, cur.pg);

  /**
   * Step 5.
   * Commit the current page
   */
  if (pgr_commit (r->pager, cur.dl.raw, cur.pg))
    {
      panic ();
    }

  /**
   * Step 6.
   * Work up the page stack and feed overflow into higher level
   * inner nodes. If none exist, push new roots
   */
  ASSERT (r->sp > 0);
  int sp = r->sp - 1;
  while (true)
    {
      /**
       * Step 1
       * Swap ofw with ofr
       */
      overflow temp = r->ofr;
      r->ofr = r->ofw;
      r->ofw = temp;

      /**
       * The last node had nothing to write.
       * We're done!
       */
      if (r->ofr.klen == 0)
        {
          return SUCCESS;
        }

      // We need a new root
      if (sp == 0)
        {
          /**
           * Scoot stack up one to reserve room for
           * a new root
           */
          ASSERT (r->sp < 20);
          for (u32 i = r->sp++; i > 0; --i)
            {
              r->stack[i] = r->stack[i - 1];
            }

          /**
           * Create a new root page with first entry
           */
          page root;
          if (pgr_new (&root, r->pager, PG_INNER_NODE))
            {
              panic ();
            }
          in_init (&root.in, r->ofr.keys[0], r->ofr.values[0], r->ofr.values[1]);
        }

      /******
       * PIN DOWN HERE
       */

      /**
       * Then copy all keys until there's no space available
       */
    }
  panic ();
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
