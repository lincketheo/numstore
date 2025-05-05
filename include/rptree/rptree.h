#pragma once

#include "intf/mm.h"
#include "intf/types.h"
#include "paging/pager.h"
#include "paging/types/hash_leaf.h"

typedef struct
{
  page page;   // Page we traversed
  p_size lidx; // Choice of the leaf we went with
} rpt_stack_v;

typedef struct
{
  b_size *keys; // len = klen
  pgno *values; // len = klen + 1
  p_size klen;
  p_size kcap;
} overflow;

typedef struct
{
  /**
   * Indexing (where am I?)
   */
  struct
  {
    bool is_open;
    bool is_seeked;
    page cur;
    b_size gidx; // The global byte I'm on
    b_size lidx; // The local byte (within the page I'm on)
  };

  /**
   * Seek result (a stack)
   */
  struct
  {
    rpt_stack_v stack[20];
    u32 sp;
  };

  /**
   * Two slots for communicating to
   * parent node. Slots store overflow keys
   * and values and switch between reading then
   * consuming
   */
  struct
  {
    overflow ofr;
    overflow ofw;
  };

  /**
   * A temporary buffer to store
   * temporary data for r+tree ops
   */
  struct
  {
    u8 *temp_mem;
    p_size tmlen;
    p_size tmcap;
  };

  /**
   * Allocates memory for temp_mem
   */
  lalloc *alloc;
  pager *pager;
} rptree;

typedef struct
{
  pager *pager;
  lalloc *alloc;
  p_size page_size;
} rpt_params;

//////////////////////////////// LIFECYCLE
/**
 * Creates the body of the rptree - doesn't actually
 * read any data.
 */
err_t rpt_create (rptree *dest, rpt_params params);

/**
 * Opens a new rptree and sets [p0]
 */
err_t rpt_new (rptree *r, pgno *p0);

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
