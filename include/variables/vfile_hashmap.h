#pragma once

#include "mm/lalloc.h"          // lalloc
#include "paging/pager.h"       // pager
#include "variables/variable.h" // vmeta

typedef struct
{
  pager *pager;
} vhashmap;

vhashmap vfhm_create (pager *p);

/**
 * Creates and initializes the base hashmap page (page 0)
 *
 * Errors:
 *   - ERR_CORRUPT - if the first page is already present
 *   - ERR_CORRUPT/ERR_IO - pgr_new errors
 */
err_t vfhm_create_hashmap (vhashmap *h, error *e);

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
    vhashmap *h,
    const variable var,
    error *e);

variable *vfhm_get (
    const vhashmap *h,
    variable *dest,
    const string key,
    error *e);
