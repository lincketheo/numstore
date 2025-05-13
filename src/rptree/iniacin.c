#include "rptree/iniacin.h"
#include "errors/error.h"
#include "intf/types.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "paging/types/hash_leaf.h"
#include "paging/types/inner_node.h"
#include "rptree/mem_inner_node.h"

/**
 * I just did this so it would
 * be similar to dliacin. I think it's probably pretty ugly
 */
typedef struct
{
  mem_inner_node input; // The input inner nodes
  mem_inner_node out;   // The output internal node
  page pg0;             // The starting page
  pager *pager;
  lalloc *alloc;
} iniacin_s;

typedef struct
{
  mem_inner_node input;
  p_size idx0;
  page pg0;
  pager *pager;
  lalloc *alloc;
} iniacin_s_params;

DEFINE_DBG_ASSERT_I (iniacin_s, iniacin_s, d)
{
  ASSERT (d);
  ASSERT (d->pg0.type == PG_INNER_NODE);
}

static inline void
iniacin_s_save_right (iniacin_s *d, p_size idx0)
{
  iniacin_s_assert (d);
  ASSERT (idx0 <= in_get_nkeys (&d->pg0.in));

  if (idx0 == in_get_nkeys (&d->pg0.in))
    {
      return;
    }

  // TODO - this can be optimized
  for (p_size i = idx0; i < in_get_nkeys (&d->pg0.in); ++i)
    {
      // Get the right key and the right page
      b_size key = in_get_key (&d->pg0.in, i);
      pgno leaf = in_get_key (&d->pg0.in, i + 1);
      if (!mintn_add_right_no_add (&d->input, key, leaf))
        {
          panic ();
        }
    }

  in_clip_right (&d->pg0.in, idx0);
}

static void
iniacin_s_create (iniacin_s *dest, iniacin_s_params p)
{
  ASSERT (dest);
  ASSERT (p.pg0.type == PG_INNER_NODE);

  *dest = (iniacin_s){
    .input = p.input,
    .out = mintn_create (p.pg0.pg),
    .pg0 = p.pg0,
    .pager = p.pager,
    .alloc = p.alloc,
  };

  iniacin_s_save_right (dest, p.idx0);

  iniacin_s_assert (dest);
}

static err_t
iniacin_s_alloc_then_write_once (iniacin_s *r, page *cur, error *e)
{
  iniacin_s_assert (r);
  ASSERT (cur);

  if (in_keys_avail (&cur->in) == 0)
    {
      // Allocate a new node
      page next;
      err_t_wrap (pgr_new (&next, r->pager, PG_INNER_NODE, e), e);

      b_size key = mintn_get_left (&r->input, cur->pg);

      /**
       * Push previous node's capacity (left key) and this node's
       * page number (right page number)
       */
      if (!mintn_add_right (&r->out, key, next.pg))
        {
          panic ();
        }

      // Commit so that we can swap it
      err_t_wrap (pgr_commit (r->pager, cur->in.raw, cur->pg, e), e);

      // Swap it
      *cur = next;
    }

  /**
   * Write as many elements to this node as possible
   */
  mintn_write_max_into_in (&cur->in, &r->input);

  return SUCCESS;
}

static err_t
iniacin_s_consume (iniacin_s *r, error *e)
{
  iniacin_s_assert (r);

  page cur = r->pg0; // Current working page

  /**
   * First,
   * Write all input data
   * and link them up correctly
   */
  while (r->input.kvlen > 0)
    {
      err_t_wrap (iniacin_s_alloc_then_write_once (r, &cur, e), e);
    }

  /**
   * Finally,
   * Link and Commit the current page
   */
  err_t_wrap (pgr_commit (r->pager, cur.in.raw, cur.pg, e), e);

  return SUCCESS;
}

err_t
iniacin (mem_inner_node *dest, iniacin_params p, error *e)
{
  iniacin_s d;
  iniacin_s_params s_p = {
    .input = p.input,
    .idx0 = p.idx0,
    .pg0 = p.pg0,
    .pager = p.pager,
    .alloc = p.alloc,
  };

  iniacin_s_create (&d, s_p);

  err_t_wrap (iniacin_s_consume (&d, e), e);

  *dest = d.out;

  return SUCCESS;
}
