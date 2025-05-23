#include "rptree/internal/seek.h"

DEFINE_DBG_ASSERT_I (seek_r, seek_r, r)
{
  ASSERT (r);
  ASSERT (r->stack);
  ASSERT (r->sp <= 20);
}

static seek_r
_rpt_seek_r_create (seek_params p)
{
  /**
   * Only allow inner node / data list starting nodes
   */
  ASSERT (p.starting_page->type & (PG_INNER_NODE | PG_DATA_LIST));

  seek_r ret = {
    .res = p.starting_page,
    .sp = 0,
  };

  seek_r_assert (&ret);

  return ret;
}

static err_t
_rpt_seek_r_push_top (seek_r *r, const page *p, p_size lidx, error *e)
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
_rpt_seek_r_push_bottom (seek_r *r, const page *p, p_size lidx, error *e)
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
   * Push res to the bottom
   */
  r->stack[0] = (seek_v){
    .pg = p->pg,
    .lidx = lidx,
  };

  return SUCCESS;
}

static err_t _rpt_seek_recurs (
    seek_r *s,
    b_size byte,
    pager *pager,
    error *e);

static inline err_t
_rpt_seek_recurs_inner_node (
    seek_r *s,
    b_size byte,
    pager *pager,
    error *e)
{
  seek_r_assert (s);

  /**
   * First, pick the index of the leaf we want
   * to traverse
   */
  p_size lidx = in_choose_lidx (&s->res->in, byte);

  /**
   * Push it onto the top of the stack
   */
  err_t_wrap (_rpt_seek_r_push_top (s, s->res, lidx, e), e);

  /**
   * Fetch the key and value at that location
   */
  pgno next = in_get_leaf (&s->res->in, lidx);
  b_size nleft = lidx > 0 ? in_get_key (&s->res->in, lidx - 1) : 0;

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
  s->res = pgr_get (
      PG_INNER_NODE | PG_DATA_LIST,
      next, pager, e);

  if (s->res == NULL)
    {
      return err_t_from (e);
    }

  return _rpt_seek_recurs (s, byte, pager, e);
}

static inline void
_rpt_seek_recurs_data_list (seek_r *s, b_size byte)
{
  seek_r_assert (s);

  /**
   * Clip the byte to the maximum of this node
   * Otherwise, set it to the requested byte
   */
  if ((p_size)byte >= dl_used (&s->res->dl))
    {
      s->lidx = dl_used (&s->res->dl);
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
_rpt_seek_recurs (seek_r *s, b_size byte, pager *pager, error *e)
{
  switch (s->res->type)
    {
    case PG_INNER_NODE:
      {
        return _rpt_seek_recurs_inner_node (s, byte, pager, e);
      }
    case PG_DATA_LIST:
      {
        _rpt_seek_recurs_data_list (s, byte);
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
_rpt_seek (seek_r *s, seek_params params, error *e)
{
  ASSERT (s);

  seek_r _s = _rpt_seek_r_create (params);
  err_t_wrap (_rpt_seek_recurs (&_s, params.whereto, params.pager, e), e);
  *s = _s;

  return SUCCESS;
}
