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
  pager pager;
  u32 page_size;
  vhash_map variables;

  struct
  {
    u32 header_size;
    u32 mpgr_len;
    i_file fp;

    // Eventually these will be specialized
    struct
    {
      lalloc *allocs;
      bool *is_present;
      u32 len;
      u32 cap;
    } allocators;

    struct
    {
      cbuffer *cbuffers;
      u32 len;
      u32 cap;
    } cbuffers;

    lalloc alloc;
  } private;
} database;

typedef struct
{
  const string fname;
  u32 page_size;
  u32 mpgr_len;
} dbcargs;

typedef struct
{
  const string fname;
} dboargs;

err_t db_create (database *dest, dbcargs args);
err_t db_open (database *dest, dboargs args);
void db_close (database *db);

// Request a new allocator
err_t db_request_alloc (
    lalloc **dest,
    database *db,
    u64 climit,
    u32 dlimit);
void db_release_alloc (
    database *db,
    lalloc *alloc);

// Request a new cbuffer
err_t db_request_cbuffer (cbuffer **dest, database *db, u32 cap);
void db_release_cbuffer (database *db, cbuffer *c);
