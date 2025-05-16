#pragma once

#include "intf/types.h"
#include "mm/lalloc.h"
#include "paging/pager.h"
#include "paging/types/hash_leaf.h"
#include "rptree/seek.h"

typedef struct
{
  /**
   * Indexing (where am I?)
   */
  struct
  {
    b_size gidx; // The global byte I'm on
    b_size lidx; // The local byte (within the page I'm on)
    page *cur;   // Current page we're on
  };

  /**
   * Seek result (a stack)
   */
  struct
  {
    seek_r seek; // Result from a seek call
    bool is_seeked;
  };

  pager *pager;
} rptree;

/**
 * Creates a new rptree and returns the page number of
 * the root node (which is a data list)
 * Errors:
 *   - From pgr_new:
 *    - ERR_CORRUPT
 *    - ERR_IO
 */
err_t rpt_open_new (rptree *dest, pgno *pg0, pager *p, error *e);

/**
 * Errors:
 *   - From pgr_get_expect
 *    - ERR_IO
 *    - ERR_CORRUPT - Expects either an inner node or data list node
 */
err_t rpt_open_existing (rptree *dest, pgno p0, pager *p, error *e);

//////////////////////////////// OPERATIONS
/**
 * Seeks to byte b
 */
err_t rpt_seek (rptree *r, b_size b, error *e);

/**
 * Reads data in chunks of size [size] into dest
 */
err_t rpt_read (
    u8 *dest,
    t_size size,
    b_size *n,
    b_size nskip,
    rptree *r,
    error *e);

/**
 * Errors:
 *   - From pgr_get_expect
 *      - ERR_CORRUPT - Expects inner nodes and data lists
 */
err_t rpt_insert (
    const u8 *src, // Source data we're writing
    t_size size,   // Size of each individual element
    b_size n,      // Number of elements to write
    rptree *r,     // The rptree that's writing this
    lalloc *alloc, // TODO - get rid of this in favor of iterative writes
    error *e       // Any error goes here
);
