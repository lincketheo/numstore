#pragma once

#include "ds/cbuffer.h"
#include "navigation.h"
#include "typing.h"
#include "vhash_map.h"

typedef struct
{
  navigator nav;

  // The currently loaded variable
  struct
  {
    vmeta meta;
    u64 size; // Cached
    bool loaded;
  };

  cbuffer *input;       // Place to fill the input data
  cbuffer *output;      // Place to store the output data
  vhash_map *variables; // In memory variable retrieval service
} cursor;

typedef struct
{
  nav_params nparams;

  cbuffer *input;
  cbuffer *output;
  vhash_map *variables;
} crsr_params;

/**
 * Returns:
 *   - Forwards errors from:
 *     - nav_create
 */
err_t crsr_create (cursor *dest, crsr_params params);

// API Commands

/**
 * Returns:
 *   - Forwards errors from:
 *     - vhash_map_get
 *     - type_bits_size
 *     - nav_goto_page
 */
err_t crsr_rewind (cursor *c, const string vname);

/**
 * Returns:
 *   - Forwards errors from:
 *     - vhash_map_get
 *     - vhash_map_insert
 *     - nav_goto_new_page
 */
err_t crsr_create_variable (cursor *c, const vcreate v);
