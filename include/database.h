#pragma once

#include "dev/errors.h"
#include "intf/io.h"
#include "sds.h"
#include "types.h"

typedef struct
{
  u32 header_size;

  // Header
  u32 page_size;
  u32 mpgr_len;
} global_config;

typedef struct
{
  // Globals
  i_file *fp;
} database;

#ifndef NTESTS
void set_global_config (u32 page_size, u32 mpgr_len);
#endif

const global_config *get_global_config (void);

err_t db_create (const string fname, u32 page_size, u32 mpgr_len);

err_t db_open (const string fname);
