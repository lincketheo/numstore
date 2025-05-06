#include "rptree/seek.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "paging/page.h"
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

  /**
   * Only allow inner node / data list starting nodes
   */
  ASSERT (p.starting_page.type & (PG_INNER_NODE | PG_DATA_LIST));

  dest->result = p.starting_page;
  dest->stack = stack;
  dest->sp = 0;
  dest->scap = p.scap;

  return SUCCESS;
}

static inline err_t
seek_r_make_room (seek_r *r, u32 newcap)
{
  seek_r_assert (r);
  seek_v *stack = lrealloc (
      r->alloc,
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
seek_r_push_page (seek_r *r, page p, p_size lidx)
{
  seek_r_assert (r);

  if (r->sp == r->scap)
    {
      // Grow by 1 because stack should usually be small
      err_t ret = seek_r_make_room (r, r->scap + 1);
      if (ret)
        {
          return ret;
        }
    }

  r->stack[r->sp++] = (seek_v){
    .pg = p.pg,
    .lidx = lidx,
  };

  return SUCCESS;
}

err_t
seek_r_push_to_bottom (seek_r *r, page p, p_size lidx)
{
  seek_r_assert (r);

  if (r->sp == r->scap)
    {
      // Grow by 1 because stack should usually be small
      err_t ret = seek_r_make_room (r, r->scap + 1);
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
    .pg = p.pg,
    .lidx = lidx,
  };

  return SUCCESS;
}

static err_t seek_recursive (seek_r *s, b_size byte, pager *pager);

static inline err_t
seek_recursive_inner_node (seek_r *s, b_size byte, pager *pager)
{
  seek_r_assert (s);
  err_t ret;

  /**
   * First, pick the index of the leaf we want
   * to traverse
   */
  p_size lidx = in_choose_lidx (&s->result.in, byte);

  /**
   * Push it onto the top of the stack
   */
  if ((ret = seek_r_push_page (s, s->result, lidx)))
    {
      return ret;
    }

  /**
   * Fetch the key and value at that location
   */
  pgno next = in_get_leaf (&s->result.in, lidx);
  b_size nleft = lidx > 0 ? in_get_key (&s->result.in, lidx - 1) : 0;

  /**
   * We are "skipping" [nleft] bytes so add that
   * to gidx and subtract it from our next query index
   * for the layer down
   *
   * This is one of the key steps of a Rope
   */
  ASSERT (byte >= nleft);
  s->gidx += nleft;
  byte -= nleft;

  /**
   * Fetch the next page, which can either be
   * an inner node or a data list
   */
  if ((ret = pgr_get_expect (
           &s->result,
           PG_INNER_NODE | PG_DATA_LIST,
           next, pager)))
    {
      return ret;
    }

  return seek_recursive (s, byte, pager);
}

static inline err_t
seek_recursive_data_list (seek_r *s, b_size byte)
{
  seek_r_assert (s);

  /**
   * Clip the byte to the maximum of this node
   * Otherwise, set it to the requested byte
   */
  if ((p_size)byte >= dl_used (&s->result.dl))
    {
      s->lidx = dl_used (&s->result.dl);
    }
  else
    {
      s->lidx = byte;
    }

  /**
   * Update global idx
   */
  s->gidx += s->lidx;

  return SUCCESS;
}

static err_t
seek_recursive (seek_r *s, b_size byte, pager *pager)
{
  switch (s->result.type)
    {
    case PG_INNER_NODE:
      {
        return seek_recursive_inner_node (s, byte, pager);
      }
    case PG_DATA_LIST:
      {
        return seek_recursive_data_list (s, byte);
      }
    default:
      {
        /**
         * pgr_get_expect protects us from this branch
         */
        ASSERT (0);
        return ERR_INVALID_STATE;
      }
    }
}

err_t
seek (seek_r *s, seek_params params)
{
  ASSERT (s);
  err_t ret = SUCCESS;

  if ((ret = seek_r_create (s, params)))
    {
      return ret;
    }

  if ((ret = seek_recursive (s, params.whereto, params.pager)))
    {
      return ret;
    }

  return SUCCESS;
}
