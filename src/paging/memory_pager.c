#include "paging/memory_pager.h"
#include "paging/memory_page.h"
#include <string.h>

// https://youtu.be/aoewwZwVmv4?list=PLSE8ODhjZXjYDBpQnSymaectKjxCy6BYq&t=1988

static inline int mpgr_find_avail(memory_pager* p)
{
  memory_pager_assert(p);
  for (int i = 0; i < BPLENGTH; ++i) {
    if (!p->is_present[i]) {
      return i;
    }
  }
  return -1;
}

static inline int mpgr_evict_page(memory_pager* p, u64 now)
{
  memory_pager_assert(p);
  u64 maxK = 0;
  int argmax = -1;
  for (int i = 0; i < BPLENGTH; ++i) {
    assert(p->is_present[i]);
    u64 this_one = mp_check(&p->pages[i], now);
    if (this_one > maxK) {
      maxK = this_one;
      argmax = i;
    }
  }
  assert(argmax >= 0);
  p->is_present[argmax] = 0;
  return argmax;
}

page* mpgr_new(memory_pager* p)
{
  memory_pager_assert(p);
  int i = mpgr_find_avail(p);
  if (i < 0) {
    i = mpgr_evict_page(p, p->clock);
  }
  memset(&p->pages[i], 0, sizeof(p->pages[i]));
  return &p->pages[i].page;
}

int mpgr_get(memory_pager* p, page* dest, page_ptr ptr)
{
  
}
