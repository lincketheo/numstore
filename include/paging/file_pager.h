#pragma once

#include "common/types.h"
#include "dev/assert.h"
#include "os/file.h"
#include "paging/page.h"

typedef struct {
  rads_file* f;
} file_pager;

static inline int file_pager_valid(const file_pager* p)
{
  return p && rads_file_valid(p->f);
}

DEFINE_ASSERT(file_pager, file_pager)

int fpgr_new(file_pager* p, page_ptr *dest);

int fpgr_delete(file_pager* p, page_ptr ptr);

int fpgr_get(file_pager* p, page_ptr ptr);
