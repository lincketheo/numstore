#pragma once

#include "mm/lalloc.h"                 // lalloc
#include "query/queries/create.h"    // create_query
#include "rptree/rptree.h"           // rptree
#include "variables/vfile_hashmap.h" // vfile_hashmap

typedef struct
{
  rptree r;
  vfile_hashmap hm;

  vmeta meta;
  bool is_meta_loaded;
} cursor;

typedef struct
{
  pager *pager;
  lalloc *alloc;
} crsr_params;

cursor crsr_create (crsr_params params);

/**
 * Errors
 *   - ERR_CORRUPT - if the first page is already present
 *   - ERR_IO - truncate failure
 *   - ERR_IO - pwrite failure
 */
err_t crsr_create_hash_table (cursor *c, error *e);

/**
 * Errors:
 *  - ERR_NOMEM -
 */
err_t crsr_create_var (cursor *c, const create_query query, error *e);

err_t crsr_create_and_load_var (cursor *c, const create_query query, error *e);

err_t crsr_load_var (cursor *c, const string vname, error *e);

void crsr_unload_var (cursor *c);
