#pragma once

#include "ast/query/queries/create.h" // create_query
#include "ast/query/query.h"
#include "mm/lalloc.h"     // lalloc
#include "rptree/rptree.h" // rptree
#include "variables/variable.h"
#include "variables/vfile_hashmap.h" // vfile_hashmap

typedef struct
{
  vfile_hashmap hm; // To retrieve / create variables

  struct
  {
    rptree r;     // To modify currently loaded variable
    variable var; // Current loaded variable
    bool is_loaded;
  };

  pager *p;
} cursor;

cursor crsr_open (pager *p);

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
err_t crsr_create_var (
    cursor *c,
    create_query *create_q,
    error *e);

/**
err_t crsr_load_var (cursor *c, const string vname, error *e);

void crsr_unload_var (cursor *c);
*/
