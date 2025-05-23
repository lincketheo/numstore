#include "rptree/internal/ini.h"

typedef struct
{
  mem_inner_node input; // The input inner nodes
  mem_inner_node out;   // The output internal node
  page *pg0;            // The starting page
  pager *pager;
  lalloc *alloc;
} ini_s;

typedef struct
{
  mem_inner_node input;
  p_size idx0;
  page *pg0;
  pager *pager;
  lalloc *alloc;
} ini_s_params;

DEFINE_DBG_ASSERT_I (ini_s, ini_s, d)
{
  ASSERT (d);
  ASSERT (d->pg0->type == PG_INNER_NODE);
}

static inline void
ini_s_save_right (ini_s *d, p_size idx0)
{
  ini_s_assert (d);
  ASSERT (idx0 <= in_get_nkeys (&d->pg0->in));

  if (idx0 == in_get_nkeys (&d->pg0->in))
    {
      return;
    }

  // TODO - this can be optimized
  for (p_size i = idx0; i < in_get_nkeys (&d->pg0->in); ++i)
    {
      // Get the right key and the right page
      b_size key = in_get_key (&d->pg0->in, i);
      pgno leaf = in_get_key (&d->pg0->in, i + 1);
      if (!mintn_add_right_no_add (&d->input, key, leaf))
        {
          panic ();
        }
    }

  in_clip_right (&d->pg0->in, idx0);
}

static void
ini_s_create (ini_s *dest, ini_s_params p)
{
  ASSERT (dest);
  ASSERT (p.pg0->type == PG_INNER_NODE);

  *dest = (ini_s){
    .input = p.input,
    .out = mintn_create (p.pg0->pg),
    .pg0 = p.pg0,
    .pager = p.pager,
    .alloc = p.alloc,
  };

  ini_s_save_right (dest, p.idx0);

  ini_s_assert (dest);
}

static err_t
ini_s_alloc_then_write_once (ini_s *r, page *cur, error *e)
{
  ini_s_assert (r);
  ASSERT (cur);

  if (in_keys_avail (&cur->in) == 0)
    {
      // Allocate a new node
      page *next = pgr_new (r->pager, PG_INNER_NODE, e);
      if (next == NULL)
        {
          return err_t_from (e);
        }

      b_size key = mintn_get_left (&r->input, cur->pg);

      /**
       * Push previous node's capacity (left key) and this node's
       * page number (right page number)
       */
      if (!mintn_add_right (&r->out, key, next->pg))
        {
          panic ();
        }

      // Commit so that we can swap it
      err_t_wrap (pgr_write (r->pager, cur, e), e);

      // Swap it
      cur = next;
    }

  /**
   * Write as many elements to this node as possible
   */
  mintn_write_max_into_in (&cur->in, &r->input);

  return SUCCESS;
}

static err_t
ini_s_consume (ini_s *r, error *e)
{
  ini_s_assert (r);

  page *cur = r->pg0; // Current working page

  /**
   * First,
   * Write all input data
   * and link them up correctly
   */
  while (r->input.kvlen > 0)
    {
      err_t_wrap (ini_s_alloc_then_write_once (r, cur, e), e);
    }

  /**
   * Finally,
   * Link and Commit the current page
   */
  err_t_wrap (pgr_write (r->pager, cur, e), e);

  return SUCCESS;
}

err_t
ini (mem_inner_node *dest, ini_params p, error *e)
{
  ini_s d;
  ini_s_params s_p = {
    .input = p.input,
    .idx0 = p.idx0,
    .pg0 = p.pg0,
    .pager = p.pager,
    .alloc = p.alloc,
  };

  ini_s_create (&d, s_p);

  err_t_wrap (ini_s_consume (&d, e), e);

  *dest = d.out;

  return SUCCESS;
}
