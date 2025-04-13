#include "paging/memory_page.h"

DEFINE_DBG_ASSERT (memory_page, memory_page, p)
{
  ASSERT (p);

  // Require monotonic increasing times from idx + 1, or 0's
  u64 prev = 0;
  for (int i = 0; i < p->idx; ++i)
    {
      int j = (p->idx + i + 1) % K;

      // Either both == 0, or cur > prev
      int valid = (p->lruk[j] == 0 && prev == 0) || (p->lruk[j] > prev);

      ASSERT (valid);
    }
}

void
mp_access (memory_page *m, u64 now)
{
  m->lruk[m->idx] = now;
  m->idx = (m->idx + 1) % K;
}

u64
mp_check (const memory_page *m, u64 now)
{
  // Note - first fillup kth = 0 (memset 0)
  // So return now - which is big, so probably evict
  u64 kth = m->lruk[(m->idx + 1) % K];
  ASSERT (now > kth);
  return now - kth;
}
