#pragma once

#include "intf/mm.h"
#include "intf/types.h"

typedef struct
{
  u8 *data;
  pgno pgno;
  int is_present;
} memory_page;

typedef struct
{
  memory_page *pages;
  u32 len;
  u32 idx;
} memory_pager;

typedef struct
{
  // Allocates constant room for:
  //  1. Room for memory_page array (len * sizeof(memory_page))
  //  2. All pages (len * page_size)
  lalloc *alloc;
  u32 len; // Number of pages to store in memory
  p_size page_size;
} mpgr_params;

/**
 * Returns:
 *  - ERR_NOMEM from allocation
 */
err_t mpgr_create (memory_pager *dest, mpgr_params params);

/**
 * Creates a new page
 * Returns:
 *   - NULL if the memory_pager is full
 */
u8 *mpgr_new (memory_pager *p, pgno pgno);

bool mpgr_is_full (const memory_pager *p);

/**
 * Gets page [pgno]
 * Returns:
 *   - NULL if the page doesn't exist
 */
u8 *mpgr_get (const memory_pager *p, pgno pgno);

/**
 * Gets the next evictable page. [p] must be full
 * Always succeeds
 */
pgno mpgr_get_evictable (const memory_pager *p);

/**
 * Evicts the page pgno. [pgno] must be in the pool
 * Always succeeds
 */
void mpgr_evict (memory_pager *p, pgno pgno);
