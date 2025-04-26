#pragma once

#include "dev/assert.h"
#include "intf/mm.h"
#include "paging/page.h"
#include "paging/pager.h"

typedef struct
{
  page page;
  u32 idn; // Numerator of the index we are on
  u32 idd; // Denominator of the index we are on
} nav_pgctx;

typedef struct
{
  nav_pgctx *stack;
  u32 sp;
  u32 scap;

  pager *pager;
  lalloc *stack_allocator;
} navigator;

typedef struct
{
  u32 stack_starting_capacity;
  pager *pager;
  lalloc *stack_allocator;
} nav_params;

/**
 * Returns:
 *   - ERR_NOMEM - can't allocate stack
 */
err_t nav_create (navigator *dest, nav_params params);
nav_pgctx *nav_top (navigator *n);

// API Commands
/**
 * All Return:
 *  - ERR_PGSTACK_OVERFLOW if the stack is overfull and can't grow
 */
err_t nav_goto_page (navigator *n, u64 pgno, page_type expect);
err_t nav_push_page (navigator *n, u64 pgno, page_type expect);
err_t nav_goto_new_page (navigator *n, page_type type);
err_t nav_push_new_page (navigator *n, page_type type);
