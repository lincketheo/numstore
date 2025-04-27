#pragma once

#include "cursor/btree_cursor.h"
#include "ds/cbuffer.h"
#include "typing.h"
#include "vhash_map.h"

typedef struct
{
  btree_cursor btc;

  // The currently loaded variable
  struct
  {
    vmeta meta;
    bool loaded;
  };

  cbuffer *input;       // Place to fill the input data
  cbuffer *output;      // Place to store the output data
  vhash_map *variables; // In memory variable retrieval service
} cursor;

typedef struct
{
  btc_params nparams;

  cbuffer *input;
  cbuffer *output;
  vhash_map *variables;
} crsr_params;

/**
 * Returns:
 *   - Forwards errors from:
 *     - btc_create
 */
err_t crsr_create (cursor *dest, crsr_params params);

/**
 * Returns:
 *   - Forwards errors from:
 *     - vhash_map_get
 *     - vhash_map_insert
 *     - btc_goto_new_page
 */
err_t crsr_create_var (cursor *c, const vcreate v);

/**
 * Returns:
 *   - Forwards errors from:
 *     - vhash_map_get
 */
err_t crsr_load_var_str (cursor *c, const string vname);

/**
 * Returns:
 *   - ERR_NOT_LOADED - no variable to unload
 */
err_t crsr_unload_var_str (cursor *c);

/**
 * Returns:
 *  - DOESNT
 *   - Forwards errors from:
 *     - vhash_map_get
 *     - type_bits_size
 *     - btc_goto_page
 */
err_t crsr_btcigate (cursor *c, u64 toloc);

/**
 * Returns:
 */
err_t crsr_read (cursor *c, u64 n, u64 step);

/**
 * Returns:
 */
err_t crsr_write (cursor *c, u64 n);

/**
 * Returns:
 */
err_t crsr_update (cursor *c, u64 n, u64 step);

/**
 * Returns:
 */
err_t crsr_delete (cursor *c, u64 n, u64 step);
