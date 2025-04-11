#pragma once

#include "dev/assert.h"
#include "paging/page.h"

#define K 10

typedef struct {
  page page;
  page_ptr ptr;
  u64 lruk[K];
  int idx;
} memory_page;

int memory_page_valid(const memory_page* p);

DEFINE_ASSERT(memory_page, memory_page)

void mp_access(memory_page* m, u64 now);

u64 mp_check(memory_page* m, u64 now);
