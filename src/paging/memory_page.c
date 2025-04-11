#include "paging/memory_page.h"

int memory_page_valid(const memory_page* p)
{
  if (p == NULL) {
    return 0;
  }

  // Require monotonic increasing times from idx + 1, or 0's
  u64 prev = 0;
  for (int i = 0; i < p->idx; ++i) {
    int j = (p->idx + i + 1) % K;

    // Either both == 0, or cur > prev
    int valid = (p->lruk[j] == 0 && prev == 0) || (p->lruk[j] > prev);

    if (!valid) {
      return 0;
    }
  }

  return 1;
}

void mp_access(memory_page* m, u64 now)
{
  m->lruk[m->idx] = now;
  m->idx = (m->idx + 1) % K;
}

u64 mp_check(memory_page* m, u64 now)
{
  // Note - first fillup kth = 0 (memset 0)
  // So return now - which is big, so probably evict
  u64 kth = m->lruk[(m->idx + 1) % K];
  assert(now > kth);
  return now - kth;
}
