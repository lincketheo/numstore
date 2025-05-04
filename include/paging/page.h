#pragma once

#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/types.h"
#include "paging/types/data_list.h"
#include "paging/types/hash_leaf.h"
#include "paging/types/hash_page.h"
#include "paging/types/inner_node.h"

typedef struct page_s page;

typedef enum
{
  PG_DATA_LIST = (1 << 0),
  PG_INNER_NODE = (1 << 1),
  PG_HASH_PAGE = (1 << 2),
  PG_HASH_LEAF = (1 << 3),
} page_type;

///////////// Generic Page type
typedef struct page_s
{
  page_type type;
  pgno pg;

  union
  {
    data_list dl;
    inner_node in;
    hash_page hp;
    hash_leaf hl;
  };
} page;

/**
 * returns:
 *  ERR_INVALID_STATE if raw data is not the type you expect
 */
err_t page_read_expect (page *dest, int type, u8 *raw, p_size rlen, pgno pg);
void page_init (page *dest, page_type type, u8 *raw, p_size rlen, pgno pg);
