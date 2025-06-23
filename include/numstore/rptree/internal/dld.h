#pragma once

#include "core/intf/types.h"                // p_size
#include "numstore/paging/page.h"           // page
#include "numstore/paging/pager.h"          // pager
#include "numstore/rptree/mem_inner_node.h" // mem_inner_node

/**
 * An internal wrapper around the rptree read algorithm
 * (for data list because that's the only layer you can read)
 *
 * DLD = Data List Delete
 */
typedef struct
{
  u8 *dest;          // Output buffer (or NULL)
  t_size size;       // Size of each element
  b_size n;          // Max Number of elements to read
  b_size nskip;      // Stride
  p_size idx0;       // The index into keys to start at
  const page *start; // Starting page
  pager *pager;      // for getting pages
} dld_params;

sb_size _rpt_dld (dld_params params, error *e);
