#pragma once

#include "dev/assert.h"
#include "os/io.h"
#include "os/types.h"
#include "paging/file_pager.h"
#include "paging/memory_pager.h"

typedef enum
{
  PG_DATA_LIST,
} page_type;

///////////////// Struct to "Allocate" and "Deallocate" pages of a file

#ifndef NDEBUG
void pgr_log_stats ();
#endif

typedef struct
{
  memory_pager mpager; // For caching
  file_pager fpager;   // For source
} pager;

DEFINE_DBG_ASSERT_H (pager, pager, p);

// Allocates space for a new page
int pgr_new (pager *p, u64 *dest);

// Retrieves page
int pgr_get (pager *p, u8 **dest, u64 ptr);

// Writes page to disk
int pgr_commit (pager *p, u64 ptr);
