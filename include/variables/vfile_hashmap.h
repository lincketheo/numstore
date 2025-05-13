#pragma once

#include "mm/lalloc.h"          // lalloc
#include "paging/pager.h"       // pager
#include "variables/variable.h" // vmeta

typedef struct
{
  pager *pager;
} vfile_hashmap;

vfile_hashmap vfhm_create (pager *p);

/**
 * Creates and initializes the base hashmap page (page 0)
 *
 * Errors:
 *   - ERR_CORRUPT - if the first page is already present
 *   - ERR_CORRUPT/ERR_IO - pgr_new errors
 */
err_t vfhm_create_hashmap (vfile_hashmap *h, error *e);

/**
 * Inserts [key] into the hash map
 *
 * Errors:
 */
err_t vfhm_insert (
    vfile_hashmap *h, // The vfhm
    const string key, // The key to insert
    vmeta value,      // The value we're inserting
    error *e          // Any errors that occur
);

err_t vfhm_update_pg0 (
    vfile_hashmap *h, // The vfhm
    const string key, // The key we're updating
    pgno pg0,         // The new page
    error *e          // Any errors that occur
);

err_t vfhm_get (
    const vfile_hashmap *h, // The vfhm
    vmeta *dest,            // The vmeta destination to write to
    const string key,       // The key we're querying
    lalloc *alloc,          // To allocate the result string and type
    error *e                // Any errors that occur
);
