#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "sds.h"
#include "types.h"

///////////////////////////// FILE PAGING

/**
 * A pager that loads pages directly from a file
 */
typedef struct
{
  u32 page_size;
  u64 npages;
  u32 header_size;
  i_file *f;
} file_pager;

DEFINE_DBG_ASSERT_H (file_pager, file_pager, p);
err_t fpgr_create (
    file_pager *dest,
    i_file *f,
    u32 page_size,
    u32 header_size);
err_t fpgr_new (file_pager *p, u64 *pgno);
err_t fpgr_get_expect (file_pager *p, u8 *dest, u64 pgno);
err_t fpgr_delete (file_pager *p, u64 pgno);
err_t fpgr_commit (file_pager *p, const u8 *src, u64 pgno);

///////////////////////////// MEMORY PAGE
typedef struct
{
  u8 *data;
  u64 pgno;
  int is_present;
} memory_page;

DEFINE_DBG_ASSERT_H (memory_page, memory_page, p);
void mp_create (memory_page *dest, u8 *data, u32 dlen, u64 pgno);

///////////////////////////// MEMORY PAGER

typedef struct
{
  memory_page *pages;
  u32 len;
  u32 idx;
} memory_pager;

DEFINE_DBG_ASSERT_H (memory_pager, memory_pager, p);
memory_pager mpgr_create (memory_page *pages, u32 len);
u8 *mpgr_new (memory_pager *p, u64 pgno);
u8 *mpgr_get (const memory_pager *p, u64 pgno);
u64 mpgr_get_evictable (const memory_pager *p);
void mpgr_evict (memory_pager *p, u64 pgno);

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
  u32 *len;    // Length of the hash table
  u64 *hashes; // Hashes pointing to header_size of linked list
} hash_page;

DEFINE_DBG_ASSERT_H (hash_page, hash_page, d);

/////////////// HASH LEAF

typedef struct
{
  u64 *next;    // Pointer to next hash map leaf
  u16 *nvalues; // Number of values in this node
  u16 *offsets; // Pointers to where each key value header_sizes
} hash_leaf;

DEFINE_DBG_ASSERT_H (hash_leaf, hash_leaf, d);

/////////////// WRAPPER

typedef struct
{
  u8 *raw;
  u64 pgno;

  u8 *header;
  union
  {
    data_list dl;
    inner_node in;
    hash_page hp;
    hash_leaf hl;
  };
} page;

err_t page_read_expect (page *dest, page_type expected, u8 *raw, u64 pgno);
void page_init (page *dest, page_type type, u8 *raw, u32 rlen, u64 pgno);

///////////////////////////// PAGER

typedef struct
{
  memory_pager mpager;
  file_pager fpager;
  u32 page_size;
} pager;

DEFINE_DBG_ASSERT_H (pager, pager, p);
pager pgr_create (
    memory_pager mpager,
    file_pager fpager,
    u32 page_size);
err_t pgr_get_expect (page *dest, page_type type, u64 pgno, pager *p);
err_t pgr_new (page *dest, pager *p, page_type type);
err_t pgr_commit (pager *p, u8 *data, u64 pgno);
