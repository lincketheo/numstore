#pragma once

#include "intf/mm.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "rptree/mem_inner_node.h"

/**
 * The rptree contains a growing
 * stack of pages and "chosen id's"
 * that it traverses on a seek. This
 * allows it easy access to it's seek
 * history. For example, when you run
 * append, you can traverse from the root
 * to the top to add a quantity to all
 * nodes on the right
 *
 * This is one stack element
 */
typedef struct
{
  page page;   // Page we traversed
  p_size lidx; // Choice of the leaf we went with
} seek_v;      // rpt stack value

typedef struct
{
  page result;   // Top most data list element
  seek_v *stack; // A stack of inner nodes and choices
  b_size gidx;   // global idx
  p_size lidx;   // local idx (within top node)
  u32 scap;      // Capacity of stack
  u32 sp;        // Stack pointer (1 past top)
} seek_r;        // A seek result

typedef struct
{
  b_size whereto;
  pager *pager;
  u32 scap;
  lalloc *alloc;
} seek_params;

err_t seek (seek_r *r, seek_params params);

void seek_insert_prepare (seek_r *r, b_size total);

typedef struct
{
  mem_inner_node input;
  lalloc *alloc;
  pager *pager;
} spup_params;

err_t seek_propagate_up (seek_r *r, spup_params params);
