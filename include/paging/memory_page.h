#pragma once

#include "paging/page.h"

#define K 10

typedef struct {
  page page;
  u64 lruk[K];
  int idx;
} memory_page;

static inline void mp_access(memory_page* m, u64 now)
{
  m->lruk[m->idx] = now;
  m->idx = (m->idx + 1) % K;
}

static inline u64 mp_check(memory_page* m, u64 now)
{
  // Note - first fillup kth = 0 (memset 0)
  // So return now - which is big, so probably evict
  u64 kth = m->lruk[(m->idx + 1) % K];
  assert(now > kth);
  return now - kth;
}
