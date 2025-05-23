#pragma once

#include "errors/error.h"          // error
#include "intf/types.h"            // f32
#include "paging/page.h"           // page
#include "paging/pager.h"          // pager
#include "rptree/mem_inner_node.h" // mem_inner_node

/**
 * An internal wrapper for the insert algorithm for one
 * data list node layer (the bottom layer)
 *
 * DLI = Data List Insert
 *
 * This function takes a starting data list page,
 * and writes to that page and allocates new pages
 * while linking them correctly until [src] is used up
 *
 * It creates a mem_inner_node that represents
 * all the key / pages we need to push upwards to
 * our parent node
 *
 * Essentially you have:
 *
 * Data list: [.......______]
 *                |  |
 *                |  Filled Here
 *              Insert Here
 *
 * This function appends to that data list at the insertion point
 * and when it gets full, it creates a new page, links them,
 * and continues writing there. While doing that, it's constructing
 * a valid "inner_node" that is just:
 *
 *       k0         k1         k2
 *  pg0       pg1       pg2           ......
 *
 *  Note pg0 IS pg0 here and pages are just page numbers, not
 *  whole pages. The intent is to pass that inner node up until
 *  it's fully consumed by parent nodes and our database is in a
 *  good spot
 */
typedef struct
{
  p_size idx0;       // What byte we are starting on in this node
  const page *start; // Starting page (should be a data list)
  pager *pager;      // Pager for creating new pages
  const u8 *src;     // Data to read from
  t_size size;       // Size of each element to consume
  b_size n;          // Number of elements to write
  f32 fill_factor;   // Percent of nodes to fill when splitting
} dli_params;

sb_size _rpt_dli (mem_inner_node *dest, dli_params params, error *e);
