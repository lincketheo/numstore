#include "rptree/seek.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "intf/mm.h"
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
seek_r_create (seek_r *dest, seek_params p, error *e)
{
  ASSERT (dest);

  lalloc_r stack = lmalloc (p.alloc, p.scap, p.scap, sizeof *dest->stack);
  if (stack.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Not enough memory to allocate rptree stack of "
          "size %d elements, %d bytes",
          p.scap, p.scap * (u32)sizeof *dest->stack);
    }

  /**
   * Only allow inner node / data list starting nodes
   */
  ASSERT (p.starting_page.type & (PG_INNER_NODE | PG_DATA_LIST));

  dest->result = p.starting_page;
  dest->stack = stack.ret;
  dest->sp = 0;
  dest->scap = p.scap;

  return SUCCESS;
}

static inline err_t
seek_r_make_room (seek_r *r, u32 newcap, error *e)
{
  seek_r_assert (r);

  lalloc_r stack = lmalloc (r->alloc, newcap, newcap, sizeof *r->stack);
  if (stack.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Not enough memory to allocate rptree stack of "
          "size %d elements, %d bytes",
          newcap, newcap * (u32)sizeof *r->stack);
    }

  r->stack = stack.ret;
  r->scap = newcap;
  return SUCCESS;
}

static err_t
seek_r_push_page (seek_r *r, page p, p_size lidx, error *e)
{
  seek_r_assert (r);

  if (r->sp == r->scap)
    {
      err_t_wrap (seek_r_make_room (r, r->scap + 1, e), e);
    }

  r->stack[r->sp++] = (seek_v){
    .pg = p.pg,
    .lidx = lidx,
  };

  return SUCCESS;
}

err_t
seek_r_push_to_bottom (seek_r *r, page p, p_size lidx, error *e)
{
  seek_r_assert (r);

  if (r->sp == r->scap)
    {
      err_t_wrap (seek_r_make_room (r, r->scap + 1, e), e);
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

static err_t seek_recursive (seek_r *s, b_size byte, pager *pager, error *e);

static inline err_t
seek_recursive_inner_node (seek_r *s, b_size byte, pager *pager, error *e)
{
  seek_r_assert (s);

  /**
   * First, pick the index of the leaf we want
   * to traverse
   */
  p_size lidx = in_choose_lidx (&s->result.in, byte);

  /**
   * Push it onto the top of the stack
   */
  err_t_wrap (seek_r_push_page (s, s->result, lidx, e), e);

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
  err_t_wrap (
      pgr_get_expect (
          &s->result,
          PG_INNER_NODE | PG_DATA_LIST,
          next, pager, e),
      e);

  return seek_recursive (s, byte, pager, e);
}

static inline void
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
}

static err_t
seek_recursive (seek_r *s, b_size byte, pager *pager, error *e)
{
  switch (s->result.type)
    {
    case PG_INNER_NODE:
      {
        return seek_recursive_inner_node (s, byte, pager, e);
      }
    case PG_DATA_LIST:
      {
        seek_recursive_data_list (s, byte);
        return SUCCESS;
      }
    default:
      {
        /**
         * pgr_get_expect protects us from this branch
         */
        crash ();
      }
    }
}

err_t
seek (seek_r *s, seek_params params, error *e)
{
  ASSERT (s);

  err_t_wrap (seek_r_create (s, params, e), e);
  err_t_wrap (seek_recursive (s, params.whereto, params.pager, e), e);

  return SUCCESS;
}
