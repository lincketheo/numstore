#pragma once

#include "dev/assert.h"
#include "paging/memory_page.h"
#include "paging/page.h"

#define BPLENGTH 100

typedef struct {
  memory_page pages[BPLENGTH];
  int is_present[BPLENGTH];
  u64 clock;
} memory_pager;

static inline int memory_pager_valid(const memory_pager* p)
{
  return p != NULL;
}

DEFINE_ASSERT(memory_pager, memory_pager)

page* mpgr_new(memory_pager* p);

int mpgr_get(memory_pager* p, page* dest, page_ptr ptr);
