#include "rptree/seek.h"
#include "dev/assert.h"
#include "dev/errors.h"

DEFINE_DBG_ASSERT_I (rpt_seek_r, rpt_seek_r, r)
{
  ASSERT (r);
  ASSERT (r->stack);
  ASSERT (r->scap > 0);
  ASSERT (r->sp <= r->scap);
}

err_t
rpts_create (rpt_seek_r *dest, rpts_params p)
{
  ASSERT (dest);

  rpts_v *stack = lmalloc (p.alloc, p.scap * sizeof *stack);
  if (!stack)
    {
      return ERR_NOMEM;
    }

  dest->stack = stack;
  dest->sp = 0;
  dest->alloc = p.alloc;
  dest->scap = p.scap;

  return SUCCESS;
}

static inline err_t
rpts_make_room (rpt_seek_r *r, u32 newcap)
{
  rpt_seek_r_assert (r);
  rpts_v *stack = lrealloc (
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

err_t
rpts_push_page (rpt_seek_r *r, page p, p_size lidx)
{
  rpt_seek_r_assert (r);

  if (r->sp == r->scap)
    {
      // Grow by 1 because stack should usually be small
      err_t ret = rpts_make_room (r, r->scap + 1);
      if (ret)
        {
          return ret;
        }
    }

  r->stack[r->sp++] = (rpts_v){
    .page = p,
    .lidx = lidx,
  };

  return SUCCESS;
}

void
rpts_reset (rpt_seek_r *r)
{
  rpt_seek_r_assert (r);
  r->sp = 0;
  r->sp = 0;
}

void
rpts_free (rpt_seek_r *r)
{
  rpt_seek_r_assert (r);
  lfree (r->alloc, r->stack);
  r->sp = 0;
  r->scap = 0;
}

u32
rpts_len (rpt_seek_r *r)
{
  rpt_seek_r_assert (r);
  ASSERT (r->sp > 0);
  return r->sp;
}

rpts_v
rpts_top (rpt_seek_r *r)
{
  rpt_seek_r_assert (r);
  ASSERT (r->sp > 0);
  return r->stack[r->sp - 1];
}

rpts_v
rpts_get (rpt_seek_r *r, u32 i)
{
  rpt_seek_r_assert (r);
  ASSERT (i < r->sp);
  return r->stack[i];
}

err_t
rpts_push_to_bottom (rpt_seek_r *r, page p, p_size lidx)
{
  rpt_seek_r_assert (r);

  if (r->sp == r->scap)
    {
      // Grow by 1 because stack should usually be small
      err_t ret = rpts_make_room (r, r->scap + 1);
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
  r->stack[0] = (rpts_v){
    .page = p,
    .lidx = lidx,
  };

  return SUCCESS;
}
