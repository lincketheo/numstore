#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/cbuffer.h"
#include "intf/io.h"
#include "intf/mm.h"
#include "intf/types.h"
#include "paging/pager.h"
#include "vhash_map.h"

//////////////// Global Database
typedef struct
{
  pager pager;
  u32 page_size;
  vhash_map variables;

  u32 header_size;
  u32 mpgr_len;
  i_file fp;

  lalloc alloc;
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

bool db_exists (const string fname);
err_t db_create_and_open (database *dest, dbcargs args);
err_t db_open (database *dest, dboargs args);
void db_close (database *db);
