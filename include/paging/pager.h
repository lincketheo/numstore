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
 * Errors:
 *    - ERR_IO - fstat failure
 *    - ERR_NOMEM - not enough memory to allocate memory pager
 *    - ERR_CORRUPT - If the file is found in a bad spot on initial open
 */
err_t pgr_create (pager *dest, pgr_params params, error *e);

/**
 * Errors:
 *    - ERR_IO - pwrite error when evicting if evicting is needed
 *    - ERR_CORRUPT - no page pgno exists (empty read)
 *    - ERR_CORRUPT - Page header does not match any of [type]
 *    - ERR_CORRUPT - extra page checks fail (<page type>_validate)
 */
err_t pgr_get_expect (page *dest, int type, pgno pgno, pager *p, error *e);

/**
 * Errors:
 *   - ERR_CORRUPT - We successfully wrote a new page, but by the time
 *        the page was written, then read again, it disappeared
 *        e.g. truncate is successful, then pread fails. This could
 *        be caused by a race condition
 *   - ERR_IO - truncate failure
 *   - ERR_IO - pwrite failure if an eviction is needed
 */
err_t pgr_new (page *dest, pager *p, page_type type, error *e);

/**
 * Errors:
 *    - ERR_IO - pwrite error on page write
 */
err_t pgr_commit (pager *p, u8 *data, pgno pgno, error *e);
