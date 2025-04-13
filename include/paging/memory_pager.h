#pragma once

#include "dev/assert.h"
#include "paging/memory_page.h"

#define BPLENGTH 1

/**
 * Memory pager is what textbooks usually call buffer pool.
 * It uses LRU-K algorithm to determine which pages to evict
 * Each memory page holds K most recently accessed times and
 * evicts the page where now - 7th previous time is greatest
 *
 * Should be initialized to 0
 */
typedef struct
{
  memory_page pages[BPLENGTH];
  int is_present[BPLENGTH];
  u64 clock; // Willing to accept overflow bug IF you exceed trillions
} memory_pager;

DEFINE_DBG_ASSERT_H (memory_pager, memory_pager, p);

// Searches for available spot, returns -1 on no space left
int mpgr_find_avail (const memory_pager *p);

// Must call find_avail first
u8 *mpgr_new (memory_pager *p, u64 ptr);

// Retrieves page ptr page must exist
u8 *mpgr_get (memory_pager *p, u64 ptr);

// Check if page exists - return the index or -1 on not available
int mpgr_check_page_exists (const memory_pager *p, u64 ptr);

// Check which page you should evict next
u64 mpgr_get_evictable (const memory_pager *p);

// Delete a page - page must exist
void mpgr_delete (memory_pager *p, u64 ptr);
