#pragma once

#include "common/types.h"
#include "dev/assert.h"
#include "os/io.h"
#include "paging/file_pager.h"
#include "paging/memory_pager.h"

///////////////// Struct to "Allocate" and "Deallocate" pages of a file

typedef struct
{
  memory_pager mpager; // For caching
  file_pager fpager;   // For source
} pager;

DEFINE_DBG_ASSERT (pager, pager, p);

// Allocates space for a new page
int pgr_new (pager *p, u64 *dest);

// Retrieves page
int pgr_get (pager *p, u8 **dest, u64 ptr);

// Writes page to disk
int pgr_commit (pager *p, u64 ptr);
