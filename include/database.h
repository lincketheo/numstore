#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "intf/mm.h"
#include "paging.h"
#include "sds.h"
#include "types.h"
#include "vhash_map.h"

//////////////// Global Database
typedef struct
{
  // PUBLIC
  pager pager;
  u32 header_size;
  u32 page_size;
  u32 mpgr_len;
  vhash_map variables;

  /////// PRIVATE
  /// Raw open resources
  struct
  {
    u8 *_data;
    i_file fp;
  } resources;

  struct
  {
    lalloc *allocs;
    bool *is_present;
    u32 len;
    u32 cap;
  } allocators;

  lalloc alloc; // To allocate internal resources
} database;

DEFINE_DBG_ASSERT_H (database, database, g);

err_t db_create (const string fname, u32 page_size, u32 mpgr_len);
err_t db_open (database *dest, const string fname);
void db_close (database *db);

lalloc *db_request_alloc (database *db, u32 limit);
void db_release_alloc (database *db, lalloc *alloc);
