#pragma once

#include "paging/file_pager.h"
#include "paging/memory_pager.h"
#include "paging/page.h"

typedef struct
{
  memory_pager mpager;
  file_pager fpager;
  u32 page_size;
} pager;

typedef struct
{
  i_file f;
  p_size page_size;
  p_size header_size;
  u32 memory_pager_len;

  lalloc *memory_pager_allocator;
} pgr_params;

/**
 * Returns
 *   - Forwards errors from mpgr_create and fpgr_create
 */
err_t pgr_create (pager *dest, pgr_params params);

/**
 * Returns
 *   - Forwards errors from
 *     - pgr_commit
 *     - fpgr_get_expect
 *     - page_read_expect
 */
err_t pgr_get_expect (page *dest, int type, pgno pgno, pager *p);

/**
 * Returns
 *   - Forwards errors from
 *     - fpgr_new
 *     - pgr_commit
 *     - fpgr_get_expect
 */
err_t pgr_new (page *dest, pager *p, page_type type);

/**
 * Returns:
 *  - Forwards errors from:
 *    - fpgr_commit
 */
err_t pgr_commit (pager *p, u8 *data, pgno pgno);
