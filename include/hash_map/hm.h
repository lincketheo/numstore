#pragma once

#include "errors/error.h"
#include "paging/pager.h"
#include "variables/variable.h"

typedef struct hm_s hm;

/**
 * Opens up a new hash map using [p] as a pager
 *
 * Errors:
 *   - ERR_NOMEM - no memory to allocate [hm]
 */
hm *hm_open (pager *p, error *e);

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
err_t hm_insert (
    hm *h,
    const variable var,
    error *e);

err_t hm_get (
    hm *h,
    variable *dest,   // Destination variable - has it's own memory
    lalloc *alloc,    // Allocates variable string and type for [dest]
    const string key, // Key to search for
    error *e);
