#pragma once

#include "bytes.h"
#include "dev/assert.h"
#include "os/io.h"
#include "os/types.h"

///////////////////////////// FILE PAGING

/**
 * A pager that loads pages directly from a file
 */
typedef struct
{
  i_file *f;
} file_pager;

static inline DEFINE_DBG_ASSERT_I (file_pager, file_pager, p)
{
  ASSERT (p);
  i_file_assert (p->f);
}

// Allocates a new page or returns err.  New page number is stored in dest
int fpgr_new (file_pager *p, u64 *dest);

// Deletes a page - Page must exist
int fpgr_delete (file_pager *p, u64 ptr);

// Gets a page - Page must exist
int fpgr_get (file_pager *p, u8 *dest_page, u64 ptr);

// Gets a page, or creates one if ptr doesn't exist
int fpgr_get_or_create (file_pager *p, u8 *dest_page, u64 *ptr);

// Writes [src] to page ptr - page must exist
int fpgr_commit (file_pager *p, const u8 *src, u64 ptr);

///////////////////////////// MEMORY PAGE
typedef struct
{
  u8 *page;
  u64 ptr;
  u64 laccess;
} memory_page;

DEFINE_DBG_ASSERT_H (memory_page, memory_page, p);

// Creates an empty memory page
void mp_create (memory_page *dest, u64 ptr, u64 clock);

// Accesses a memory page
void mp_access (memory_page *m, u64 now);

// Returns time since this page was accessed
u64 mp_check (const memory_page *m, u64 now);

///////////////////////////// MEMORY PAGER

/**
 * Memory pager is what textbooks usually call buffer pool.
 * It uses LRU-K algorithm to determine which pages to evict
 * Each memory page holds K most recently accessed times and
 * evicts the page where now - 7th previous time is greatest
 */
typedef struct
{
  memory_page *pages;
  u32 len;         // len of [pages]
  int *is_present; // Array of bools if page[i] is present
  u64 clock;       // Overflow negligible
} memory_pager;

DEFINE_DBG_ASSERT_H (memory_pager, memory_pager, p);

memory_pager mpgr_create (memory_page *pages, u32 len);

// Searches for available spot, returns -1 on no space left
int mpgr_find_avail (const memory_pager *p);

// Must call find_avail first
u8 *mpgr_new (memory_pager *p, u64 ptr);

// Retrieves page ptr page must exist
u8 *mpgr_get_by_ptr (memory_pager *p, u64 ptr);

// Retrieves page ptr page must exist
u8 *mpgr_get_by_idx (memory_pager *p, u32 idx);

// Check if page exists - return the index or -1 on not available
int mpgr_check_page_exists (const memory_pager *p, u64 ptr);

// Check which page you should evict next
u64 mpgr_get_evictable (const memory_pager *p);

// Delete a page - page must exist
void mpgr_delete (memory_pager *p, u64 ptr);

// Updates page number - oldptr must exist
void mpgr_update_pgnum (memory_pager *p, u64 oldptr, u64 newptr);

///////////////////////////// PAGE TYPES

typedef enum
{
  PG_DATA_LIST,
} page_type;

typedef struct
{
  u8 *page;

  u8 *header;     // Header defining what type of node it is
  u64 *next;      // Pointer to the next node or 0
  u16 *len_num;   // Numerator of the length of this node
  u16 *len_denom; // Denominator of the length of this node

  bytes data;
} data_list;

DEFINE_DBG_ASSERT_H (data_list, data_list, d);

int dl_read_page (data_list *dest, u8 *page);

void dl_read_and_init_page (data_list *dest, u8 *page);
