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
    page cur;    // Current page we're on
    bool is_open;
  };

  /**
   * Seek result (a stack)
   */
  struct
  {
    seek_r seek; // Result from a seek call
    bool is_seeked;
  };

  lalloc espace_alloc; // Allocator for execution space
  pager *pager;
} rptree;

typedef struct
{
  pager *pager;
  lalloc *alloc; // The allocator that we're going to take space from
} rpt_params;

err_t rpt_create (rptree *dest, rpt_params params, error *e);

/**
 * Creates a new rptree and returns the page number of
 * the root node (which is a data list)
 * Errors:
 *   - From pgr_new:
 *    - ERR_CORRUPT
 *    - ERR_IO
 */
err_t rpt_new (pgno *dest, rptree *r, error *e);

/**
 * Errors:
 *   - From pgr_get_expect
 *    - ERR_IO
 *    - ERR_CORRUPT - Expects either an inner node or data list node
 */
err_t rpt_open (rptree *r, pgno p0, error *e);

/**
 * Closes rptree [r]. r should be open
 */
void rpt_close (rptree *r);

//////////////////////////////// OPERATIONS
/**
 * Seeks to byte b
 */
err_t rpt_seek (rptree *r, b_size b, error *e);

/**
 * Reads data in chunks of size [size] into dest
 */
err_t rpt_read (u8 *dest, t_size size, b_size *n, b_size nskip, rptree *r, error *e);

/**
 * Errors:
 *   - From pgr_get_expect
 *      - ERR_CORRUPT - Expects inner nodes and data lists
 */
err_t rpt_insert (const u8 *src, t_size size, b_size n, rptree *r, error *e);
