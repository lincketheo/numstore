#pragma once

#include "config.h"      // MEMORY_PAGE_LEN
#include "intf/types.h"  // bool
#include "paging/page.h" // page

typedef struct memory_pager_s memory_pager;

/**
 * Creates a new memory pager
 *
 * Returns:
 *    - ERR_NOMEM - Fails to allocate data structures
 */
memory_pager *mpgr_open (error *e);
void mpgr_close (memory_pager *mp);

/**
 * Creates a new page if there's room. Doesn't
 * write any data to the page, just "allocates" room.
 * The next step would likely be reading in a page from
 * disk
 *
 * page type on return is PG_UNKNOWN
 *
 * Returns:
 *   - NULL if the memory_pager is full, sets
 *    [evictable] to the next evictable page
 *
 * TODO - what if evictable is being used
 */
page *mpgr_new (memory_pager *p, pgno pg, pgno *evictable);

/**
 * Iterates through pages in memory_pager and removes is_present
 */
page *mpgr_pop (memory_pager *p);

/**
 * Gets page [pgno]
 *
 * If no such page exists, returns NULL,
 *
 * - if no page exists and there's room for
 *   a new page, evictable gets -1
 * - if no page exists and there's no room for
 *   a new page, evictable is the next evictable page
 */
page *mpgr_get (memory_pager *p, pgno pg, spgno *evictable);

/**
 * Evicts the page pgno. [pgno] must be in the pool
 * Always succeeds
 */
void mpgr_evict (memory_pager *p, pgno pgno);
