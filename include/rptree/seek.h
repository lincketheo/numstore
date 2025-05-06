#pragma once

#include "intf/mm.h"
#include "paging/page.h"

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
} rpts_v;      // rpt stack value

typedef struct
{
  rpts_v *stack;
  b_size gidx;
  p_size lidx;
  u32 scap;
  u32 sp;
  lalloc *alloc;
} rpt_seek_r; // rpt seek result

typedef struct
{
  u32 scap;
  lalloc *alloc;
} rpts_params;

err_t rpts_create (rpt_seek_r *dest, rpts_params p);
err_t rpts_push_page (rpt_seek_r *r, page p, p_size lidx);
void rpts_reset (rpt_seek_r *r);
void rpts_free (rpt_seek_r *r);
u32 rpts_len (rpt_seek_r *r);
rpts_v rpts_top (rpt_seek_r *r);
rpts_v rpts_get (rpt_seek_r *r, u32 i);
err_t rpts_push_to_bottom (rpt_seek_r *r, page p, p_size lidx);

/**
 * A seek operation
 */
