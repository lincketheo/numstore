#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "sds.h"
#include "types.h"

///////////////////////////// FILE PAGING

typedef struct page_s page;

/**
 * A pager that loads pages directly from a file
 */
typedef struct
{
  u32 page_size;
  u64 npages;
  u32 header_size;
  i_file f;
} file_pager;

typedef struct
{
  u8 *data;
  u64 pgno;
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
  memory_pager mpager;
  file_pager fpager;
  u32 page_size;
} pager;

typedef enum
{
  PG_DATA_LIST = 1,
  PG_INNER_NODE = 2,
  PG_HASH_PAGE = 3,
  PG_HASH_LEAF = 4,
} page_type;

pager pgr_create (memory_pager mpager, file_pager fpager, u32 page_size);
err_t pgr_get_expect (page *dest, page_type type, u64 pgno, pager *p);
err_t pgr_new (page *dest, pager *p, page_type type);
err_t pgr_commit (pager *p, u8 *data, u64 pgno);

/**
 * ============ PAGE START
 * HEADER
 * NKEYS
 * LENN (numerator)
 * LEND (denominator)
 * DATA0
 * DATA1
 * DATA2
 * ...
 * DATA(LENN / LEND)
 * 0
 * 0
 * ============ PAGE END
 */
typedef struct
{
  i64 *next;      // Pointer to the next node or -1
  u16 *len_num;   // Numerator of the length of this node
  u16 *len_denom; // Denominator of the length of this node
  u8 *data;       // The raw contiguous data pointer
} data_list;

/**
 * ============ PAGE START
 * HEADER
 * NKEYS
 * LEAF0
 * LEAF1
 * ...
 * LEAF(NKEYS)
 * 0
 * 0
 * 0
 * ...
 * 0
 * 0
 * 0
 * KEY(NKEYS - 1)
 * ...
 * KEY1
 * KEY0
 * ============ PAGE END
 */
typedef struct
{
  u16 *nkeys; // Number of keys
  u64 *leafs; // len(leafs) == nkeys + 1
  u64 *keys;  // The keys used for rope traversal
} inner_node;

/**
 * ============ PAGE START
 * HEADER
 * LEN
 * HASH0
 * HASH1
 * ...
 * HASH(LEN - 1)
 * 0
 * 0
 * ============ PAGE END
 */
typedef struct
{
  u32 *len;    // Length of the hash table
  u64 *hashes; // Hashes pointing to header_size of linked list
} hash_page;

/**
 * ============ DATA START
 * STRLEN
 * STR0
 * STR1
 * ...
 * STR(STRLEN - 1)
 * PGNO
 * TSTRLEN
 * TSTR0
 * TSTR1
 * ...
 * TSTR(TSTRLEN)
 * ============ DATA END
 */
typedef struct
{
  u16 *strlen;  // The length of the variable name
  char *str;    // The name of the variable
  u64 *pg0;     // Page 0 of the variable
  u16 *tstrlen; // The length of the type string
  char *tstr;   // The type string
} hash_leaf_tuple;

/**
 * ============ PAGE START
 * HEADER
 * NEXT
 * NVALUES
 * OFFSET0
 * OFFSET1
 * ...
 * OFFSET(NVALUES - 1)
 * 0
 * 0
 * ...
 * 0
 * 0
 * TPL2
 * TPL1
 * TPL0
 * ============ PAGE END
 */
typedef struct
{
  u64 *next;    // Pointer to next hash map leaf - 0 if none
  u16 *nvalues; // Number of values in this node
  u16 *offsets; // Pointers to where each key value header_sizes
} hash_leaf;

struct page_s
{
  u8 *header;

  u8 *raw;
  u32 len;

  u64 pgno;

  union
  {
    data_list dl;
    inner_node in;
    hash_page hp;
    hash_leaf hl;
  };
};

err_t page_read_expect (page *dest, page_type expected, u8 *raw, u32 rlen, u64 pgno);

void page_init (page *dest, page_type type, u8 *raw, u32 rlen, u64 pgno);

//// UTILS

/**
 * returns:
 *  ERR_INVALID_STATE
 */
err_t hl_get_tuple (
    hash_leaf_tuple *dest,
    const page *hl,
    u16 idx);

/**
 * returns:
 *  ERR_INVALID_STATE
 */
err_t hp_get_hash (u64 *dest, page *p, const string string);
