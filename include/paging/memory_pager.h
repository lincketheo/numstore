#pragma once

#include "config.h"      // MEMORY_PAGE_LEN
#include "intf/types.h"  // bool
#include "paging/page.h" // page

typedef struct
{
  page page;
  bool is_present;
} memory_page;

typedef struct
{
  memory_page pages[MEMORY_PAGE_LEN];
  u32 idx; // A rolling index - not very sophisticated yet
} memory_pager;

/**
 * Creates a new page
 * Returns:
 *   - NULL if the memory_pager is full
 */
page *mpgr_new (memory_pager *p, pgno pgno);

bool mpgr_is_full (const memory_pager *p);

/**
 * Gets page [pgno]
 * Returns:
 *   - NULL if the page doesn't exist
 */
page *mpgr_get_rw (memory_pager *p, pgno pgno);
const page *mpgr_get_r (const memory_pager *p, pgno pgno);

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
