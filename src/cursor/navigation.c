#include "cursor/navigation.h"
#include "dev/errors.h"
#include "intf/mm.h"

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
nav_goto_page (navigator *n, u64 pgno, page_type expect)
{
  navigator_assert (n);

  nav_pgctx *top;
  if (n->sp > 0)
    {
      top = &n->stack[n->sp - 1];
    }
  else
    {
      return nav_push_page (n, pgno, expect);
    }

  err_t ret;
  if ((ret = pgr_get_expect (&top->page, expect, pgno, n->pager)))
    {
      return ret;
    }

  top->idn = 0;
  top->idd = 1;

  return SUCCESS;
}

err_t
nav_push_page (navigator *n, u64 pgno, page_type expect)
{
  navigator_assert (n);

  err_t ret;
  if ((ret = nav_resize (n, n->sp + 1)))
    {
      return ret;
    }

  n->sp++;

  return nav_goto_page (n, pgno, expect);
}

err_t
nav_goto_new_page (navigator *n, page_type type)
{
  navigator_assert (n);

  nav_pgctx *top = &n->stack[n->sp - 1];

  err_t ret;
  if ((ret = pgr_new (&top->page, n->pager, type)))
    {
      return ret;
    }

  top->idn = 0;
  top->idd = 1;

  return SUCCESS;
}

err_t
nav_push_new_page (navigator *n, page_type type)
{
  navigator_assert (n);

  err_t ret;
  if ((ret = nav_resize (n, n->sp + 1)))
    {
      return ret;
    }

  n->sp++;

  return nav_goto_new_page (n, type);
}

nav_pgctx *
nav_top (navigator *n)
{
  navigator_assert (n);
  return &n->stack[n->sp - 1];
}
