#include "numstore/rptree/internal/ini.h"

#include "core/intf/stdlib.h"
#include "numstore/paging/pager.h"            // TODO
#include "numstore/paging/types/inner_node.h" // TODO
#include "numstore/rptree/mem_inner_node.h"   // TODO

typedef struct
{
  mem_inner_node temp;  // Stores the right half of the first node
  mem_inner_node input; // The input inner nodes
  mem_inner_node out;   // The output internal node
  page *pg0;            // The starting page
  pager *pager;
  lalloc *alloc;
} ini_s;

DEFINE_DBG_ASSERT_I (ini_s, ini_s, d)
{
  ASSERT (d);
  ASSERT (d->pg0->type == PG_INNER_NODE);
}

// Save the right half of [d] first page to temp
static inline void
ini_s_save_right (ini_s *d, p_size idx0)
{
  ini_s_assert (d);

  // idx0 is to the left or equal to key length
  ASSERT (idx0 <= in_get_nkeys (&d->pg0->in));

  // [temp] has no keys
  i_memset (&d->temp, 0, sizeof d->temp);

  // On the right most key - nothing to do
  if (idx0 == in_get_nkeys (&d->pg0->in))
    {
      return;
    }

  // Save the left value

  meminode_kv right_input = meminode_get_right (&d->input);
  meminode_create (&d->temp, right_input.value);

  for (p_size i = idx0; i < in_get_nkeys (&d->pg0->in); ++i)
    {
      // Get the right key and the right page
      b_size key = in_get_key (&d->pg0->in, i);
      pgno leaf = in_get_leaf (&d->pg0->in, i + 1);
      ASSERT (key > right_input.key);
      key -= right_input.key;
      meminode_push_right_no_add (&d->temp, key, leaf);
    }

  in_clip_right (&d->pg0->in, idx0);
}

static void
ini_s_create (ini_s *dest, ini_params p)
{
  ASSERT (dest);
  ASSERT (p.start->type == PG_INNER_NODE);

  page *page = pgr_make_writable (p.pager, p.start);

  *dest = (ini_s){
    .input = p.input,
    // meminode_create(out)
    .pg0 = page,
    .pager = p.pager,
  };
  meminode_create (&dest->out, p.start->pg);

  ini_s_save_right (dest, p.idx0);

  ini_s_assert (dest);
}

static err_t
ini_s_alloc_then_write_once (ini_s *r, mem_inner_node *input, const page *cur, error *e)
{
  ini_s_assert (r);
  ASSERT (cur);

  /**
   * This inner node is full.
   *
   * General flow:
   * 1. Allocate a new node
   * 2. Append the left most key to the current [output]
   *    memory inner node to boil up the chain
   * 3. Save and swap cur with the next node
   */
  if (in_keys_avail (&cur->in) == 0)
    {
      // Allocate a new node
      const page *next = pgr_new (r->pager, PG_INNER_NODE, e);
      if (next == NULL)
        {
          return err_t_from (e);
        }

      // Pop leftmost key and add to output for higher up the chain
      meminode_kv kv = meminode_pop_left (input, cur->pg);
      meminode_push_right (&r->out, kv.key, next->pg);

      cur = next;
    }

  // Write as many elements to this node as possible
  page *last = pgr_make_writable (r->pager, cur);
  meminode_write_max (&last->in, input);

  return SUCCESS;
}

static err_t
ini_s_consume (ini_s *r, error *e)
{
  ini_s_assert (r);

  page *cur = r->pg0; // Current working page

  // Write out all the input elements from the user supplied input
  while (r->input.klen > 0)
    {
      err_t_wrap (ini_s_alloc_then_write_once (r, &r->input, cur, e), e);
    }

  // Write out all the input elements from the user supplied input
  while (r->temp.klen > 0)
    {
      err_t_wrap (ini_s_alloc_then_write_once (r, &r->temp, cur, e), e);
    }

  return SUCCESS;
}

err_t
_rpt_ini (mem_inner_node *dest, ini_params p, error *e)
{
  ini_s d;
  ini_s_create (&d, p);

  err_t_wrap (ini_s_consume (&d, e), e);

  *dest = d.out;

  return SUCCESS;
}
