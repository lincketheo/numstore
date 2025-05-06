#pragma once

#include "intf/mm.h"
#include "intf/types.h"
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
    page cur;
    bool is_open;
  };

  /**
   * Seek result (a stack)
   */
  struct
  {
    seek_r seek;
    bool is_seeked;
  };

  lalloc *alloc; // For now one global allocator
  pager *pager;
} rptree;

typedef struct
{
  pager *pager;
  lalloc *alloc;
} rpt_params;

//////////////////////////////// LIFECYCLE
/**
 * Creates the body of the rptree - doesn't actually
 * read any data.
 */
rptree rpt_create (rpt_params params);

/**
 * Opens a new rptree
 */
err_t rpt_new (rptree *r);

/**
 * Opens an existing rptree at [p0]
 */
err_t rpt_open (rptree *r, pgno p0);

/**
 * Closes rptree [r]. r should be open
 */
void rpt_close (rptree *r);

//////////////////////////////// OPERATIONS
/**
 * Seeks to byte b
 */
err_t rpt_seek (rptree *r, b_size b);

/**
 * Reads data in chunks of size [size] into dest
 */
err_t rpt_read (u8 *dest, t_size size, b_size *n, b_size nskip, rptree *r);
err_t rpt_insert (const u8 *src, t_size size, b_size n, rptree *r);
