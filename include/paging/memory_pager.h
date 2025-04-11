#pragma once

#include "dev/assert.h"
#include "paging/memory_page.h"
#include "paging/page.h"

#define BPLENGTH 100

/**
 * Memory pager is what textbooks usually call buffer pool.
 * It uses LRU-K algorithm to determine which pages to evict
 * Each memory page holds K most recently accessed times and
 * evicts the page where now - 7th previous time is greatest
 *
 * Should be initialized to 0
 */
typedef struct {
  memory_page pages[BPLENGTH];
  int is_present[BPLENGTH];
  u64 clock;
} memory_pager;

int memory_pager_valid(const memory_pager* p);

DEFINE_ASSERT(memory_pager, memory_pager)

// Allocates space for a new page,
// possibly evicting a page if memory is limited
page* mpgr_new(memory_pager* p);

// Retrieves page ptr or returns NULL
page* mpgr_get(memory_pager* p, page_ptr ptr);
