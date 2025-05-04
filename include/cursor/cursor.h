#pragma once

/**
#include "cursor/btree_cursor.h"
#include "ds/cbuffer.h"
#include "intf/mm.h"
#include "query/queries/create.h"
#include "typing.h"
#include "variable.h"
#include "vhash_map.h"

typedef struct
{
  btree_cursor btc;
} cursor;

typedef struct
{
  btc_params nparams;
  lalloc *vtype_allocator;
} crsr_params;
*/

/**
 * Returns:
 *   - Forwards errors from:
 *     - btc_create
 */
// err_t crsr_create (cursor *dest, crsr_params params);

/**
 * Returns:
 *   - Forwards errors from:
 *     - vhash_map_get
 *     - vhash_map_insert
 *     - btc_goto_new_page
 */
// err_t crsr_create_var (cursor *c, const create_query query);

/**
 * Returns:
 *   - Forwards errors from:
 *     - vhash_map_get
 *     - vhash_map_insert
 *     - btc_goto_new_page
 */
// err_t crsr_load_var (cursor *c, vmeta *dest, const string vname);
// err_t crsr_unload_var (cursor *c);
