#include "cursor/btree_cursor.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "paging/page.h"
#include "paging/pager.h"

/**
DEFINE_DBG_ASSERT_I (btcigator, btcigator, n)
{
  ASSERT (n);
}

err_t
btc_create (btcigator *dest, btc_params params)
{
  btc_pgctx *stack = lmalloc (
      params.stack_allocator,
      params.stack_starting_capacity * sizeof *stack);

  if (!stack)
    {
      return ERR_NOMEM;
    }

  dest->stack = stack;
  dest->sp = 0;
  dest->scap = params.stack_starting_capacity;
  dest->stack_allocator = params.stack_allocator;
  dest->pager = params.pager;

  btcigator_assert (dest);

  return SUCCESS;
}

btc_pgctx *
btc_top (btcigator *n)
{
  btcigator_assert (n);
  return &n->stack[n->sp - 1];
}

// Resize page stack if needed to accomodate [cap]
static inline err_t
btc_resize (btcigator *n, u32 cap)
{
  btcigator_assert (n);

  // Check if we need to resize the stack
  if (cap > n->scap)
    {
      // Add 2 - stack never gets too big
      btc_pgctx *stack = lrealloc (
          n->stack_allocator,
          n->stack,
          (n->scap + 2) * sizeof *stack);
      if (!stack)
        {
          return ERR_PGSTACK_OVERFLOW;
        }
      n->stack = stack;
      n->scap = (n->scap + 2);
    }

  return SUCCESS;
}

err_t
btc_rewind (btcigator *n, u64 pgno, u64 size)
{
  btcigator_assert (n);

  err_t ret;

  // Ensure there's enough space
  if ((ret = btc_resize (n, 1)))
    {
      return ret;
    }

  // Get then push the root page
  page top;
  if ((ret = pgr_get_expect (&top, PG_INNER_NODE, pgno, n->pager)))
    {
      return ret;
    }

  // Reset the stack
  n->sp = 0;
  n->stack[n->sp++] = (btc_pgctx){
    .page = top,
    .idn = 0,
    .idd = 1,
  };
  n->size = size;
  n->pgno = top.pgno;

  return SUCCESS;
}

err_t
btc_new_root (btcigator *n, u64 *pgno, u64 size)
{
  btcigator_assert (n);
  ASSERT (pgno);

  err_t ret;

  page new_page;
  if ((ret = pgr_new (&new_page, n->pager, PG_INNER_NODE)))
    {
      return ret;
    }
  if ((ret = btc_rewind (n, new_page.pgno, size)))
    {
      // TODO - should I rollback the new page here?
      return ret;
    }

  *pgno = new_page.pgno;

  return SUCCESS;
}

err_t
btc_btcigate (btcigator *n, u64 idx)
{
  btcigator_assert (n);
  ASSERT (0 == idx);
  return ERR_FALLBACK;
}
*/
