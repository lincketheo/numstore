#pragma once

#include "common/types.h"
#include "dev/assert.h"
#include "os/file.h"
#include "paging/file_pager.h"
#include "paging/memory_pager.h"
#include "paging/page.h"

///////////////// Struct to "Allocate" and "Deallocate" pages of a file

typedef struct {
  memory_pager mpager; // For caching
  file_pager fpager;   // For source
} page_manager;

static inline int page_manager_valid(const page_manager* p)
{
  return p && memory_pager_valid(&p->mpager) && file_pager_valid(&p->fpager);
}

DEFINE_ASSERT(page_manager, page_manager)

// Allocates space for a new page
int pmgr_new(page_manager* p, page_ptr* dest);

// Deletes page
int pmgr_delete(page_manager* p, page_ptr ptr);

// Retrieves page
int pmgr_get(page_manager* p, page* dest, page_ptr ptr);

// Writes page to disk
int pmgr_commit(page_manager* p, const page* src);
