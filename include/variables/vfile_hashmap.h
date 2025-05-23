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
 *    - ERR_NOMEM - can't allocate type type string
 *      for serialized [var.type]
 *    - ERR_IO, ERR_CORRUPT
 *        - pgr_get_expect
 *        - pgr_new
 *        - pgr_write
 */
err_t vfhm_insert (
    vfile_hashmap *h,
    const variable var,
    lalloc *alloc,
    error *e);

err_t vfhm_get (
    const vfile_hashmap *h,
    variable *dest,
    const string key,
    lalloc *alloc,
    error *e);
