#pragma once

#include "intf/mm.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "rptree/mem_inner_node.h"

/**
 * A stack entry contains an inner node page and
 * the index of the child we chose to traverse
 */
typedef struct
{
  pgno pg;     // The page number of the page we traversed
  p_size lidx; // Choice of the leaf we went with
} seek_v;      // rpt stack value

/**
 * A "seek result" is the response to a seek call
 * on a rpt tree.
 *
 * It contains:
 * 1. A resulting Data List page
 * 2. Where we are in the global database (gidx)
 * 3. Where we are in the local page (lidx)
 * 4. Stack (and length) of previous pages we visited to get here
 */
typedef struct
{
  page result;   // The data list page that contains our seek byte
  b_size gidx;   // global location
  p_size lidx;   // local idx (within top node)
  seek_v *stack; // A stack of previous inner nodes and choices
  u32 sp;        // Length of the stack
  u32 scap;      // Capacity of the stack
  lalloc *alloc; // Allocator for growing the stack
} seek_r;

err_t seek_r_push_to_bottom (seek_r *r, page p, p_size lidx);

typedef struct
{
  page starting_page; // Starting root page
  b_size whereto;     // Which byte to seek to
  u32 scap;           // Starting capacity to allocate for the stack
  pager *pager;       // To find pages
  lalloc *alloc;      // Allocator for stack
} seek_params;

err_t seek (seek_r *r, seek_params params);

typedef struct
{
  mem_inner_node input;
  lalloc *alloc;
  pager *pager;
} spup_params;

err_t seek_propagate_up (seek_r *r, spup_params params);
