#pragma once

#include "core/ds/strings.h"   // string
#include "core/errors/error.h" // error
#include "core/intf/types.h"   // pgno

#include "numstore/config.h"                  // PAGE_SIZE
#include "numstore/paging/types/data_list.h"  // TODO
#include "numstore/paging/types/hash_leaf.h"  // TODO
#include "numstore/paging/types/hash_page.h"  // TODO
#include "numstore/paging/types/inner_node.h" // TODO

typedef struct page_s page;

typedef enum
{
  PG_DATA_LIST = (1 << 0),
  PG_INNER_NODE = (1 << 1),
  PG_HASH_PAGE = (1 << 2),
  PG_HASH_LEAF = (1 << 3),
} page_type;

#define PG_ANY (PG_DATA_LIST | PG_INNER_NODE | PG_HASH_LEAF | PG_HASH_PAGE)

///////////// Generic Page type
typedef struct page_s
{
  u8 raw[PAGE_SIZE];

  page_type type; // Redundant because it's in the header, but ok
  pgno pg;        // the page number this page belongs to

  union
  {
    data_list dl;
    inner_node in;
    hash_page hp;
    hash_leaf hl;
  };
} page;

/**
 * Errors:
 *   - ERR_CORRUPT if raw data is not the type you expect
 *     or specific page type validation checks fail
 */
err_t page_set_ptrs_expect_type (
    page *p,  // The page with raw data, no page type meta
    int type, // An Or'd int of expected page_type's
    error *e  // Accumulated error
);

/**
 * Initializes the page with [type]
 * There may be extra initialization steps for
 * the desired page type
 */
void page_init (page *p, page_type type);

/**
 * Log a friendly version of page
 */
void i_log_page (const page *p);
