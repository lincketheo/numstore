#pragma once

#include "mm/lalloc.h"
#include "paging/pager.h"
#include "variables/variable.h"

typedef struct
{
  pager *pager;
  lalloc *alloc;
} vfile_hashmap;

typedef struct
{
  pager *pager;
  lalloc *alloc;
} vfhm_params;

vfile_hashmap vfhm_create (vfhm_params params);

/**
 * Errors
 *   - ERR_CORRUPT - if the first page is already present
 *   - ERR_IO - truncate failure
 *   - ERR_IO - pwrite failure
 */
err_t vfhm_create_hashmap (vfile_hashmap *h, error *e);

err_t vfhm_insert (vfile_hashmap *h, const string key, vmeta value, error *e);

err_t vfhm_update_pg0 (vfile_hashmap *h, const string key, pgno pg0, error *e);

err_t vfhm_get (const vfile_hashmap *h, vmeta *dest, const string key, error *e);
