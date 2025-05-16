#pragma once

#include "paging/file_pager.h"   // file_pager
#include "paging/memory_pager.h" // memory_pager
#include "paging/page.h"         // page

typedef struct
{
  memory_pager mpager;
  file_pager fpager;
} pager;

/**
 * Errors (on return NULL):
 *    - ERR_IO - fstat failure
 *    - ERR_NOMEM - not enough memory to allocate pager
 *    - ERR_CORRUPT - If the file is found in a bad spot on initial open
 */
pager *pgr_create (const string fname, error *e);

/**
 * Errors (on return NULL):
 *    - ERR_IO - pwrite error when evicting if evicting is needed
 *    - ERR_CORRUPT - no page pgno exists (empty read)
 *    - ERR_CORRUPT - Page header does not match any of [type]
 *    - ERR_CORRUPT - extra page checks fail (<page type>_validate)
 */
const page *pgr_get_expect_r (int type, pgno pgno, pager *p, error *e);
page *pgr_get_expect_rw (int type, pgno pgno, pager *p, error *e);

/**
 * Errors (on return NULL):
 *   - ERR_CORRUPT - We successfully wrote a new page, but by the time
 *        the page was written, then read again, it disappeared
 *        e.g. truncate is successful, then pread fails. This could
 *        be caused by a race condition
 *   - ERR_IO - truncate failure
 *   - ERR_IO - pwrite failure if an eviction is needed
 */
page *pgr_new (pager *p, page_type type, error *e);

/**
 * Errors:
 *    - ERR_IO - pwrite error on page write
 */
err_t pgr_write (pager *p, const page *pg, error *e);
