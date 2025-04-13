#include "paging/memory_pager.h"
#include "common/macros.h"
#include "common/types.h"
#include "paging/memory_page.h"

#include <string.h>

// https://youtu.be/aoewwZwVmv4?list=PLSE8ODhjZXjYDBpQnSymaectKjxCy6BYq&t=1988

DEFINE_DBG_ASSERT (memory_pager, memory_pager, p)
{
  ASSERT (p);

  u64 used_pages[BPLENGTH];
  for (int i = 0; i < BPLENGTH; ++i)
    {
      if (!p->is_present[i])
        {
          continue;
        }

      memory_page_assert (&p->pages[i]);

      // Check that this page hasn't been used before
      u64 next = p->pages[i].ptr;
      for (int j = 0; j < i; ++j)
        {
          ASSERT (next != used_pages[j]);
        }
      used_pages[i] = next;
    }
}

int
mpgr_find_avail (const memory_pager *p)
{
  memory_pager_assert (p);

  // TODO - OPTIMIZATION - This could be a hash map
  for (int i = 0; i < BPLENGTH; ++i)
    {
      if (!p->is_present[i])
        {
          return i;
        }
    }
  return -1;
}

u8 *
mpgr_new (memory_pager *p, u64 ptr)
{
  memory_pager_assert (p);

  int i = mpgr_find_avail (p);
  ASSERT (i >= 0);

  // TODO - OPTIMIZATION I might be able to remove this
  memset (&p->pages[i], 0, sizeof (p->pages[i]));
  p->pages[i].ptr = ptr;
  p->is_present[i] = 1;

  return p->pages[i].page;
}

u8 *
mpgr_get (memory_pager *p, u64 ptr)
{
  int idx = mpgr_check_page_exists (p, ptr);
  ASSERT (idx >= 0);
  ASSERT (idx < BPLENGTH);
  return p->pages[idx].page;
}

int
mpgr_check_page_exists (const memory_pager *p, u64 ptr)
{
  for (int i = 0; i < BPLENGTH; ++i)
    {
      if (p->is_present[i] && p->pages[i].ptr == ptr)
        {
          return i;
        }
    }
  return -1;
}

/**
 * Evicts the page with the greatest time between now and
 * kth last access. Returns the index of the evicted page
 */
int
mpgr_get_evictable (const memory_pager *p, u64 now)
{
  memory_pager_assert (p);

  u64 maxK = 0;
  int argmax = -1;
  for (int i = 0; i < BPLENGTH; ++i)
    {
      ASSERT (p->is_present[i]);
      u64 this_one = mp_check (&p->pages[i], now);
      if (this_one > maxK)
        {
          maxK = this_one;
          argmax = i;
        }
    }

  ASSERT (argmax >= 0);
  ASSERT (argmax < K);

  return argmax;
}

void
mpgr_delete (memory_pager *p, u64 ptr)
{
  int idx = mpgr_check_page_exists (p, ptr);
  ASSERT (idx >= 0);
  ASSERT (idx < BPLENGTH);
  p->is_present[idx] = 0;
}
