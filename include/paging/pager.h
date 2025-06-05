#pragma once

#include "paging/file_pager.h"   // file_pager
#include "paging/memory_pager.h" // memory_pager
#include "paging/page.h"         // page

// An opaque pager type
typedef struct pager_s pager;

/**
 * Errors (on return NULL):
 *    - ERR_IO - fstat failure
 *    - ERR_NOMEM - not enough memory to allocate pager
 *    - ERR_CORRUPT - If the file is found in a bad spot on initial open
 */
pager *pgr_open (const string fname, error *e);
err_t pgr_close (pager *p, error *e);

/**
 * Fetch a page, expect it to be any of the unioned types in [type]
 * or else return ERR_CORRUPT
 *
 * Errors (on return NULL):
 *    - ERR_IO - pwrite error when evicting if evicting is needed
 *    - ERR_CORRUPT - no page pgno exists (empty read)
 *    - ERR_CORRUPT - Page header does not match any of [type]
 *    - ERR_CORRUPT - extra page checks fail (<page type>_validate)
 */
const page *pgr_get (int type, pgno pgno, pager *p, error *e);

/**
 * Errors (on return NULL):
 *   - ERR_CORRUPT - We successfully wrote a new page, but by the time
 *        the page was written, then read again, it disappeared
 *        e.g. truncate is successful, then pread fails. This could
 *        be caused by a race condition
 *   - ERR_IO - truncate failure
 *   - ERR_IO - pwrite failure if an eviction is needed
 */
const page *pgr_new (pager *p, page_type type, error *e);

/**
 * Converts a constant read only page to a writable page
 *
 * Errors (on return NULL)
 *    - TODO
 */
page *pgr_make_writable (pager *p, const page *pg);

/**
 * Releases page
 */
void pgr_release (pager *p, const page *pg);

/**
 * Errors:
 *    - ERR_IO - pwrite error on page write
 */
err_t pgr_save (pager *p, const page *pg, error *e);
