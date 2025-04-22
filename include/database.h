#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "intf/mm.h"
#include "paging.h"
#include "sds.h"
#include "types.h"

///////////////////////////////// All the Globals Live Here

/**
 * Global applications state
 * - Any global app state lives in this module.
 * - App state is seperated into two objects:
 *   1. config: This holds configuration things that the
 *   program reads in at bootup. In the future, these might
 *   be able to change, but for now, they should be treated as
 *   constant
 *   2. database: This holds all the large data structures
 *   that are alive in memory
 *
 * Global Design Philosophy:
 * - So far, nothing references this module. Any system that
 *   requires, e.g. page_size, should require you to pass page_size
 *   directly to it, not using global app state.
 */

//////////////// Global Config
typedef struct
{
  u32 header_size;

  // Header
  u32 page_size;
  u32 mpgr_len;
} global_config;

DEFINE_DBG_ASSERT_H (global_config, global_config, g);

//////////////// Global Database
typedef struct
{
  pager pager;
  memory_pager mpgr;
  file_pager fpgr;
  lalloc alloc;

  struct
  {
    u8 *_data;
    i_file fp;
  } resources;
} global_database;

DEFINE_DBG_ASSERT_H (global_database, global_database, g);
err_t db_create (const string fname, u32 page_size, u32 mpgr_len);
err_t db_open (const string fname);
void db_close ();
