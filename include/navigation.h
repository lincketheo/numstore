#pragma once

#include "database.h"
#include "dev/assert.h"
#include "paging.h"

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

  lalloc *stack_allocator;
  pager *pager;
} navigator;

err_t nav_create (navigator *dest, database *db);
nav_pgctx *nav_top (navigator *n);

// API Commands
err_t nav_goto_page (navigator *n, u64 pgno, page_type expect);
err_t nav_push_page (navigator *n, u64 pgno, page_type expect);
err_t nav_goto_new_page (navigator *n, page_type type);
err_t nav_push_new_page (navigator *n, page_type type);
