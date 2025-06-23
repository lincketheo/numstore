#pragma once

#include "core/intf/types.h"       // pgno
#include "numstore/paging/page.h"  // page
#include "numstore/paging/pager.h" // pager

/**
 * An internal wrapper around the seek algorithm
 * for rptree
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
  const page *res;  // The data list page that contains our seek byte
  b_size gidx;      // global location
  p_size lidx;      // local idx (within top node)
  seek_v stack[20]; // A stack of previous inner nodes and choices
  u32 sp;           // Length of the stack
} seek_r;

/**
 * Errors:
 *  - ERR_NOMEM - not enough memory to grow the stack
 */
err_t _rpt_seek_r_push_bottom (
    seek_r *r,
    const page *p,
    p_size lidx,
    error *e);

typedef struct
{
  const page *starting_page; // Starting root page
  b_size whereto;            // Which byte to seek to
  u32 scap;                  // Starting capacity to allocate for the stack
  pager *pager;              // To find pages
} seek_params;

/**
 * Errors:
 *   - ERR_NOMEM - not enough memory to create or grow the stack
 *   - From pgr_get_expect:
 *      - ERR_CORRUPT - We're only interested in data lists and inner nodes
 *      - ERR_IO
 */
err_t _rpt_seek (seek_r *r, seek_params params, error *e);
