#pragma once

#include "common/types.h"
#include "dev/assert.h"
#include "memory/page.h"
#include "os/file.h"

///////////////// Struct to "Allocate" and "Deallocate" pages of a file

typedef struct {
  rads_file* f;
} page_alloc;

static inline int page_alloc_valid(const page_alloc* p)
{
  return rads_file_valid(p->f);
}

DEFINE_ASSERT(page_alloc, page_alloc)

int page_alloc_new(page_alloc* p, page* dest);

int page_alloc_delete(page_alloc* p, const page* src);

int page_alloc_get(page_alloc* p, page* dest);

int page_alloc_update(page_alloc* p, const page* src);
