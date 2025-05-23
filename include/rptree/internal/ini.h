#pragma once

#include "intf/types.h"            // p_size
#include "paging/page.h"           // page
#include "paging/pager.h"          // pager
#include "rptree/mem_inner_node.h" // mem_inner_node

/**
 * An internal wrapper around the rptree insert algorithm
 * for inner nodes (the higher layers) - splitting inner nodes
 * and saving changes etc
 *
 * INI = Inner Node Insert
 *
 * See DLI
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
  p_size idx0;          // The index into keys to start at
  const page *start;    // Starting page
  pager *pager;         // for getting pages
  mem_inner_node input; // Input from layer below
  f32 fill_factor;      // Percent to fill node on split
} ini_params;

err_t _rpt_ini (mem_inner_node *dest, ini_params params, error *e);
