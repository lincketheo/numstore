#include "paging/memory_pager.h"
#include "common/macros.h"
#include "common/types.h"
#include "paging/memory_page.h"

#include <string.h>

// https://youtu.be/aoewwZwVmv4?list=PLSE8ODhjZXjYDBpQnSymaectKjxCy6BYq&t=1988

int memory_pager_valid(const memory_pager* p)
{
  if (p == NULL) {
    return 0;
  }

  page_ptr used_pages[BPLENGTH];
  for (int i = 0; i < BPLENGTH; ++i) {
    if (!p->is_present[i]) {
      continue;
    }

    // Check that underlying page is valid
    if (!memory_page_valid(&p->pages[i])) {
      return 0;
    }

    // Check that this page hasn't been used before
    page_ptr next = p->pages[i].ptr;
    for (int j = 0; j < i; ++j) {
      if (next == used_pages[j]) {
        return 0;
      }
    }
    used_pages[i] = next;
  }
  return 1;
}

/**
 * Searches for an available spot, if none are available, returns -1
 */
private
inline int mpgr_find_avail(memory_pager* p)
{
  memory_pager_assert(p);
  for (int i = 0; i < BPLENGTH; ++i) {
    if (!p->is_present[i]) {
      return i;
    }
  }
  return -1;
}

/**
 * Evicts the page with the greatest time between now and
 * kth last access
 */
private
inline int mpgr_evict_page(memory_pager* p, u64 now)
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

page* mpgr_get(memory_pager* p, page_ptr ptr)
{
  for (int i = 0; i < BPLENGTH; ++i) {
    if (p->is_present[i] && p->pages[i].ptr == ptr) {
      return &p->pages[i].page;
    }
  }
  return NULL;
}
