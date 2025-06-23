#pragma once

#include "core/errors/error.h"              // error
#include "core/intf/types.h"                // f32
#include "numstore/paging/page.h"           // page
#include "numstore/paging/pager.h"          // pager
#include "numstore/rptree/mem_inner_node.h" // mem_inner_node

/**
 * An internal wrapper for the write algorithm for one
 * data list node layer
 *
 * DLW = Data List Write
 */
typedef struct
{
  p_size idx0;       // What byte we are starting on in this node
  const page *start; // Starting page (should be a data list)
  pager *pager;      // Pager for creating new pages
  const u8 *src;     // Data to read from
  t_size size;       // Size of each element to consume
  b_size n;          // Number of elements to write
  b_size nskip;      // Stride
} dlw_params;

sb_size _rpt_dlw (dlw_params params, error *e);
