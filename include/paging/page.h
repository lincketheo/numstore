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
  PG_DATA_LIST = 1,
  PG_INNER_NODE = 2,
  PG_HASH_PAGE = 3,
  PG_HASH_LEAF = 4,
} page_type;

///////////// Generic Page type
typedef struct page_s
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
} page;

typedef struct
{
  page_type type;
  u8 *raw;
  u32 page_size;
  u64 pgno;
} page_interpret_params;

/**
 * returns:
 *  ERR_INVALID_STATE if raw data is not the type you expect
 */
err_t page_read_expect (page *dest, page_interpret_params params);
void page_init (page *dest, page_interpret_params params);
