#include "rptree/seek.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "mm/lalloc.h"
#include "paging/page.h"
#include "paging/types/data_list.h"
#include "paging/types/inner_node.h"
#include "rptree/iniacin.h"

DEFINE_DBG_ASSERT_I (seek_r, seek_r, r)
{
  ASSERT (r);
  ASSERT (r->stack);
  ASSERT (r->sp <= 20);
}

static seek_r
seek_r_create (seek_params p)
{
  /**
   * Only allow inner node / data list starting nodes
   */
  ASSERT (p.starting_page->type & (PG_INNER_NODE | PG_DATA_LIST));

  seek_r ret = {
    .result = p.starting_page,
    .sp = 0,
  };

  seek_r_assert (&ret);

  return ret;
}

static err_t
seek_r_push_page (seek_r *r, page *p, p_size lidx, error *e)
{
  seek_r_assert (r);

  if (r->sp == 20)
    {
      return error_causef (
          e, ERR_PAGE_STACK_OVERFLOW,
          "Seek: Page stack overflow");
    }

  r->stack[r->sp++] = (seek_v){
    .pg = p->pg,
    .lidx = lidx,
  };

  return SUCCESS;
}

err_t
seek_r_push_to_bottom (seek_r *r, page *p, p_size lidx, error *e)
{
  seek_r_assert (r);

  if (r->sp == 20)
    {
      return error_causef (
          e, ERR_PAGE_STACK_OVERFLOW,
          "Seek: Page stack overflow");
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
    .pg = p->pg,
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
  p_size lidx = in_choose_lidx (&s->result->in, byte);

  /**
   * Push it onto the top of the stack
   */
  err_t_wrap (seek_r_push_page (s, s->result, lidx, e), e);

  /**
   * Fetch the key and value at that location
   */
  pgno next = in_get_leaf (&s->result->in, lidx);
  b_size nleft = lidx > 0 ? in_get_key (&s->result->in, lidx - 1) : 0;

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
  s->result = pgr_get_expect_rw (
      PG_INNER_NODE | PG_DATA_LIST, next, pager, e);
  if (s->result == NULL)
    {
      return err_t_from (e);
    }

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
  if ((p_size)byte >= dl_used (&s->result->dl))
    {
      s->lidx = dl_used (&s->result->dl);
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
  switch (s->result->type)
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
        UNREACHABLE ();
      }
    }
}

err_t
seek (seek_r *s, seek_params params, error *e)
{
  ASSERT (s);

  seek_r _s = seek_r_create (params);
  err_t_wrap (seek_recursive (&_s, params.whereto, params.pager, e), e);
  *s = _s;

  return SUCCESS;
}
