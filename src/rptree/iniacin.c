#include "rptree/iniacin.h"
#include "dev/errors.h"
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

static inline err_t
iniacin_s_save_right (iniacin_s *d, p_size idx0)
{
  iniacin_s_assert (d);
  ASSERT (idx0 <= in_get_nkeys (&d->pg0.in));

  if (idx0 == in_get_nkeys (&d->pg0.in))
    {
      return SUCCESS;
    }

  err_t ret = SUCCESS;

  // TODO - this can be optimized
  for (p_size i = idx0; i < in_get_nkeys (&d->pg0.in); ++i)
    {
      // Get the right key and the right page
      b_size key = in_get_key (&d->pg0.in, i);
      pgno leaf = in_get_key (&d->pg0.in, i + 1);
      if ((ret = mintn_add_right_no_add (&d->input, key, leaf)))
        {
          return ret;
        }
    }

  in_clip_right (&d->pg0.in, idx0);

  return ret;
}

static err_t
iniacin_s_create (iniacin_s *dest, iniacin_s_params p)
{
  ASSERT (dest);
  ASSERT (p.pg0.type == PG_INNER_NODE);

  *dest = (iniacin_s){
    .input = p.input,
    .out = mintn_create (
        (mintn_params){
            .alloc = p.alloc,
            .pg = p.pg0.pg,
        }),
    .pg0 = p.pg0,
    .pager = p.pager,
    .alloc = p.alloc,
  };

  err_t ret = iniacin_s_save_right (dest, p.idx0);
  if (ret)
    {
      return ret;
    }

  iniacin_s_assert (dest);

  return SUCCESS;
}

static err_t
iniacin_s_alloc_then_write_once (iniacin_s *r, page *cur)
{
  iniacin_s_assert (r);
  ASSERT (cur);

  err_t ret = SUCCESS;

  if (in_keys_avail (&cur->in) == 0)
    {
      // Allocate a new node
      page next;
      if ((ret = pgr_new (&next, r->pager, PG_INNER_NODE)))
        {
          goto failed;
        }

      b_size key = mintn_get_left (&r->input, cur->pg);

      /**
       * Push previous node's capacity (left key) and this node's
       * page number (right page number)
       */
      if ((ret = mintn_add_right (&r->out, key, next.pg)))
        {
          return ret;
        }

      // Commit so that we can swap it
      if ((ret = pgr_commit (r->pager, cur->in.raw, cur->pg)))
        {
          return ret;
        }

      // Swap it
      *cur = next;
    }

  /**
   * Write as many elements to this node as possible
   */
  mintn_write_max_into_in (&cur->in, &r->input);

  return SUCCESS;

failed:
  return ret;
}

static err_t
iniacin_s_consume (iniacin_s *r)
{
  iniacin_s_assert (r);

  err_t ret = SUCCESS; // Return code

  page cur = r->pg0; // Current working page

  /**
   * First,
   * Write all input data
   * and link them up correctly
   */
  while (r->input.kvlen > 0)
    {
      ret = iniacin_s_alloc_then_write_once (r, &cur);

      if (ret)
        {
          goto theend;
        }
    }

  /**
   * Finally,
   * Link and Commit the current page
   */
  if ((ret = pgr_commit (r->pager, cur.in.raw, cur.pg)))
    {
      goto theend;
    }

theend:
  return ret;
}

static err_t
iniacin_s_complete (mem_inner_node *dest, iniacin_s *d)
{
  iniacin_s_assert (d);
  mintn_free (&d->input);

  // TODO
  // Probably don't need to clip because I think
  // later on we append more
  err_t ret = SUCCESS;
  if ((ret = mintn_clip (&d->out)))
    {
      return ret;
    }

  *dest = d->out;
  return ret;
}

err_t
iniacin (mem_inner_node *dest, iniacin_params p)
{
  iniacin_s d;
  err_t ret = SUCCESS;

  if ((ret = iniacin_s_create (
           &d, (iniacin_s_params){
                   .input = p.input,
                   .idx0 = p.idx0,
                   .pg0 = p.pg0,
                   .pager = p.pager,
                   .alloc = p.alloc,
               })))
    {
      return ret;
    }

  if ((ret = iniacin_s_consume (&d)))
    {
      return ret;
    }

  if ((ret = iniacin_s_complete (dest, &d)))
    {
      return ret;
    }

  return ret;
}
