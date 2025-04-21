#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "paging.h"
#include "sds.h"
#include "types.h"

///////////////////////////////// All the Globals Live Here
//////////////// Global Config
typedef struct
{
  u32 header_size;

  // Header
  u32 page_size;
  u32 mpgr_len;
} global_config;

DEFINE_DBG_ASSERT_H (global_config, global_config, g);
void set_global_config (u32 page_size, u32 mpgr_len);
const global_config *get_global_config (void);

//////////////// Global Database
typedef struct
{
  pager pager;
  memory_pager mpgr;
  file_pager fpgr;

  struct
  {
    u8 *_data;
    i_file *fp;
  } resources;
} global_database;

DEFINE_DBG_ASSERT_H (global_database, global_database, g);
global_database *get_global_database (void);
err_t db_create (const string fname, u32 page_size, u32 mpgr_len);
err_t db_open (const string fname);
void db_close ();
