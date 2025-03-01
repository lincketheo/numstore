#pragma once

#include "dev/assert.h"
#include "intf/io.h"
#include "sds.h"
#include "types.h"

///////////////////////////// FILE PAGING

/**
 * A pager that loads pages directly from a file
 */
typedef struct
{
  i_file *f;
} file_pager;

DEFINE_DBG_ASSERT_H (file_pager, file_pager, p);
int fpgr_new (file_pager *p, u64 *dest);
int fpgr_delete (file_pager *p, u64 ptr);
int fpgr_get (file_pager *p, u8 *dest_page, u64 ptr);
int fpgr_get_or_create (file_pager *p, u8 *dest_page, u64 *ptr);
int fpgr_commit (file_pager *p, const u8 *src, u64 ptr);

///////////////////////////// MEMORY PAGE
typedef struct
{
  u8 *page;
  u64 ptr;
  u64 laccess;
} memory_page;

DEFINE_DBG_ASSERT_H (memory_page, memory_page, p);
void mp_create (memory_page *dest, u64 ptr, u64 clock);
void mp_access (memory_page *m, u64 now);
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
int mpgr_find_avail (const memory_pager *p);
u8 *mpgr_new (memory_pager *p, u64 ptr);
u8 *mpgr_get_by_ptr (memory_pager *p, u64 ptr);
u8 *mpgr_get_by_idx (memory_pager *p, u32 idx);
int mpgr_check_page_exists (const memory_pager *p, u64 ptr);
u64 mpgr_get_evictable (const memory_pager *p);
void mpgr_delete (memory_pager *p, u64 ptr);
void mpgr_update_pgnum (memory_pager *p, u64 oldptr, u64 newptr);

///////////////////////////// PAGER

typedef struct
{
  memory_pager mpager;
  file_pager fpager;
} pager;

DEFINE_DBG_ASSERT_H (pager, pager, p);

///////////////////////////// PAGE TYPES

typedef enum
{
  PG_DATA_LIST = 1,
  PG_INNER_NODE = 2,
  PG_HASH_PAGE = 3,
  PG_HASH_LEAF = 4,
} page_type;

/////////////// DATA LIST

typedef struct
{
  i64 *next;      // Pointer to the next node or -1
  u16 *len_num;   // Numerator of the length of this node
  u16 *len_denom; // Denominator of the length of this node
  u8 *data;       // The raw contiguous data pointer
} data_list;

/////////////// INNER NODE

typedef struct
{
  u16 *nkeys; // Number of keys
  u64 *leafs; // len(leafs) == nkeys + 1
  u64 *keys;  // The keys used for rope traversal
} inner_node;

DEFINE_DBG_ASSERT_H (inner_node, inner_node, d);

/////////////// HASH PAGE

typedef struct
{
  i64 *hashes; // Hashes pointing to start of linked list
} hash_page;

DEFINE_DBG_ASSERT_H (hash_page, hash_page, d);

/////////////// HASH LEAF

typedef struct
{
  u64 *next;    // Pointer to next hash map leaf
  u16 *nvalues; // Number of values in this node
  u16 *offsets; // Pointers to where each key value starts
} hash_leaf;

DEFINE_DBG_ASSERT_H (hash_leaf, hash_leaf, d);

/////////////// WRAPPER

typedef struct
{
  u8 *raw;

  u8 *header;
  union
  {
    data_list dl;
    inner_node in;
    hash_page hp;
    hash_leaf hl;
  };
} page;

int page_read_expect (page *dest, page_type expected, u8 *raw);
void page_init (page *dest, page_type type, u8 *raw);
