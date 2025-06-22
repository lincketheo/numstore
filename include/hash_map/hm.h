#pragma once

#include "errors/error.h"
#include "paging/pager.h"
#include "variables/variable.h"

/**
 * The File base hash map. Wraps a pager
 * and uses the first page as hash indexes, which
 * link to pages of hash "leaf" data
 */
typedef struct hm_s hm;

/**
 * Opens up a new hash map using [p] as a pager
 *
 * Errors:
 *   - ERR_NOMEM - no memory to allocate [hm]
 */
hm *hm_open (pager *p, error *e);
void hm_close (hm *h);

err_t hm_get (
    hm *h,
    variable *dest,   // Destination variable - has it's own memory
    lalloc *alloc,    // Allocates variable string and type for [dest]
    const string key, // Key to search for
    error *e);

/**
 * Inserts [var] into the hash map
 *
 * Errors:
 *   - ERR_NOMEM - var.type is too large to allocate a temporary buffer
 *   - ERR_IO, ERR_CORRUPT - inherited from:
 *      - pgr_read
 *      - pgr_new
 *      - pgr_write
 */
err_t hm_insert (hm *h, const variable var, error *e);

err_t hm_delete (hm *h, const string vname, error *e);
