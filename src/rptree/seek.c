#include "rptree/seek.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "paging/types/data_list.h"
#include "paging/types/inner_node.h"
#include "rptree/iniacin.h"

DEFINE_DBG_ASSERT_I (seek_r, seek_r, r)
{
  ASSERT (r);
  ASSERT (r->stack);
  ASSERT (r->scap > 0);
  ASSERT (r->sp <= r->scap);
}

static err_t
seek_r_create (seek_r *dest, seek_params p)
{
  ASSERT (dest);

  seek_v *stack = lmalloc (
      p.alloc, p.scap * sizeof *stack);
  if (!stack)
    {
      return ERR_NOMEM;
    }

  dest->stack = stack;
  dest->sp = 0;
  dest->scap = p.scap;

  return SUCCESS;
}

static inline err_t
seek_r_make_room (seek_r *r, u32 newcap, lalloc *alloc)
{
  seek_r_assert (r);
  seek_v *stack = lrealloc (
      alloc,
      r->stack,
      newcap * sizeof *stack);

  if (!stack)
    {
      return ERR_NOMEM;
    }

  r->stack = stack;
  r->scap = newcap;
  return SUCCESS;
}

static err_t
seek_r_push_page (seek_r *r, page p, p_size lidx, lalloc *alloc)
{
  seek_r_assert (r);

  if (r->sp == r->scap)
    {
      // Grow by 1 because stack should usually be small
      err_t ret = seek_r_make_room (r, r->scap + 1, alloc);
      if (ret)
        {
          return ret;
        }
    }

  r->stack[r->sp++] = (seek_v){
    .page = p,
    .lidx = lidx,
  };

  return SUCCESS;
}

/**
static void
seek_reset (seek_r *r)
{
  seek_r_assert (r);
  r->sp = 0;
  r->sp = 0;
}

static void
seek_r_free (seek_r *r, lalloc *alloc)
{
  seek_r_assert (r);
  lfree (alloc, r->stack);
  r->sp = 0;
  r->scap = 0;
}
*/

static u32
seek_r_len (seek_r *r)
{
  seek_r_assert (r);
  ASSERT (r->sp > 0);
  return r->sp;
}

/**
static seek_v
seek_r_top (seek_r *r)
{
  seek_r_assert (r);
  ASSERT (r->sp > 0);
  return r->stack[r->sp - 1];
}
*/

static seek_v
seek_r_get (seek_r *r, u32 i)
{
  seek_r_assert (r);
  ASSERT (i < r->sp);
  return r->stack[i];
}

static err_t
seek_r_push_to_bottom (seek_r *r, page p, p_size lidx, lalloc *alloc)
{
  seek_r_assert (r);

  if (r->sp == r->scap)
    {
      // Grow by 1 because stack should usually be small
      err_t ret = seek_r_make_room (r, r->scap + 1, alloc);
      if (ret)
        {
          return ret;
        }
    }

  /**
   * Scoot stack up 1 to make room
   */
  for (u32 i = r->sp++; i > 0; --i)
    {
      r->stack[i] = r->stack[i - 1];
    }

  /**
   * Push result to the bottom
   */
  r->stack[0] = (seek_v){
    .page = p,
    .lidx = lidx,
  };

  return SUCCESS;
}

typedef struct
{
  page cur;
  seek_r ret;
  lalloc *alloc;
  pager *pager;
} seek_r_stateful;

static err_t
seek_r_stateful_create (
    seek_r_stateful *dest, seek_params p)
{
  ASSERT (dest);

  err_t ret = seek_r_create (&dest->ret, p);
  if (ret)
    {
      return ret;
    }
  dest->alloc = p.alloc;
  dest->pager = p.pager;
  return SUCCESS;
}

static err_t
seek_recursive (seek_r_stateful *s, b_size byte)
{
  err_t ret = SUCCESS;

  switch (s->cur.type)
    {
    case PG_INNER_NODE:
      {
        // Choose which leaf to traverse to next
        p_size lidx = in_choose_lidx (&s->cur.in, byte);

        // Push it to the top of the stack
        if ((ret = seek_r_push_page (&s->ret, s->cur, lidx, s->alloc)))
          {
            return ret;
          }

        // Fetch key and value at that location
        pgno next = in_get_leaf (&s->cur.in, lidx);
        b_size nleft = 0;
        if (lidx > 0)
          {
            nleft = in_get_key (&s->cur.in, lidx - 1);
          }

        // Subtract left quantity from byte query
        ASSERT (byte >= nleft);
        s->ret.gidx += nleft;
        byte -= nleft;

        // Fetch the next page
        if ((ret = pgr_get_expect (
                 &s->cur,
                 PG_INNER_NODE | PG_DATA_LIST,
                 next, s->pager)))
          {
            return ret;
          }

        // Recursive call
        return seek_recursive (s, byte);
      }
    case PG_DATA_LIST:
      {
        if ((p_size)byte >= dl_used (&s->cur.dl))
          {
            // Clip byte to the last byte of this node
            s->ret.lidx = dl_used (&s->cur.dl);
          }
        else
          {
            // Set local id to byte
            s->ret.lidx = byte;
          }

        s->ret.result = s->cur;
        s->ret.gidx += s->ret.lidx;

        return SUCCESS;
      }
    default:
      {
        return ERR_INVALID_STATE;
      }
    }
}

err_t
seek (seek_r *r, seek_params params)
{
  ASSERT (r);
  err_t ret = SUCCESS;

  seek_r_stateful s;
  if ((ret = seek_r_stateful_create (&s, params)))
    {
      return ret;
    }

  if ((ret = seek_recursive (&s, params.whereto)))
    {
      return ret;
    }

  *r = s.ret;

  return SUCCESS;
}

void
seek_insert_prepare (seek_r *r, b_size total)
{
  for (u32 i = 0; i < seek_r_len (r); ++i)
    {
      seek_v s = seek_r_get (r, i);
      in_add_right (&s.page.in, s.lidx, total);
    }
}

err_t
seek_propagate_up (seek_r *r, spup_params p)
{
  seek_r_assert (r);

  ASSERT (seek_r_len (r) > 0);
  u32 sp = seek_r_len (r) - 1; // Start at the top - work down

  err_t ret = SUCCESS;

  mem_inner_node in = p.input;
  while (in.kvlen > 0)
    {
      page cur;    // The next page we want to update
      p_size from; // The key index inside cur we want to append to

      if (sp == 0)
        {
          if (pgr_new (&cur, p.pager, PG_INNER_NODE))
            {
              return ret;
            }

          if (seek_r_push_to_bottom (r, cur, 0, p.alloc))
            {
              return ret;
            }
          from = 0; // Start from the beginning
        }
      else
        {
          seek_v v = r->stack[--sp];
          cur = v.page;
          from = v.lidx;
        }

      mem_inner_node out;
      ret = iniacin (
          &out, (iniacin_params){
                    .input = in,
                    .idx0 = from,
                    .alloc = p.alloc,
                    .pager = p.pager,
                    .pg0 = cur,
                });
      if (ret)
        {
          return ret;
        }
      in = out;
    }

  return ret;
}
