#pragma once

#include "intf/types.h"
#include "paging/pager.h"
#include "rptree/mem_inner_node.h"

/**
 * INIACIN = Inner Node Insert Allocate and Create Inner Node
 *
 * Read DLIACIN.
 *
 * This function takes an Inner Node this time and "feeds" it
 * an in memory inner node while it allocates new pages and
 * creates new inner nodes
 *
 *       k0         k1         k2
 *  pg0       pg1       pg2           ......
 *
 */
typedef struct
{
  p_size idx0; // The current page we are replacing
  page pg0;
  pager *pager;
  lalloc *alloc;
  mem_inner_node input;
  // TODO - fill factor - what percent to fill when overflow
} iniacin_params;

err_t iniacin (mem_inner_node *dest, iniacin_params params);
