#include "cursor/navigation.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "paging/page.h"
#include "paging/pager.h"

DEFINE_DBG_ASSERT_I (navigator, navigator, n)
{
  ASSERT (n);
}

err_t
nav_create (navigator *dest, nav_params params)
{
  nav_pgctx *stack = lmalloc (
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

  navigator_assert (dest);

  return SUCCESS;
}

nav_pgctx *
nav_top (navigator *n)
{
  navigator_assert (n);
  return &n->stack[n->sp - 1];
}

// Resize page stack if needed to accomodate [cap]
static inline err_t
nav_resize (navigator *n, u32 cap)
{
  navigator_assert (n);

  // Check if we need to resize the stack
  if (cap > n->scap)
    {
      // Add 2 - stack never gets too big
      nav_pgctx *stack = lrealloc (
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
nav_rewind (navigator *n, u64 pgno, u64 size)
{
  navigator_assert (n);

  err_t ret;

  // Ensure there's enough space
  if ((ret = nav_resize (n, 1)))
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
  n->stack[n->sp++] = (nav_pgctx){
    .page = top,
    .idn = 0,
    .idd = 1,
  };
  n->size = size;
  n->pgno = top.pgno;

  return SUCCESS;
}

err_t
nav_new_root (navigator *n, u64 *pgno, u64 size)
{
  navigator_assert (n);
  ASSERT (pgno);

  err_t ret;

  page new_page;
  if ((ret = pgr_new (&new_page, n->pager, PG_INNER_NODE)))
    {
      return ret;
    }
  if ((ret = nav_rewind (n, new_page.pgno, size)))
    {
      // TODO - should I rollback the new page here?
      return ret;
    }

  *pgno = new_page.pgno;

  return SUCCESS;
}

err_t
nav_navigate (navigator *n, u64 idx)
{
  navigator_assert (n);
  ASSERT (0 == idx);
  return ERR_FALLBACK;
}
