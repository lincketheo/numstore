#include "paging/memory_pager.h"

#include "dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "dev/testing.h" // TEST
#include "intf/io.h"     // i_malloc
#include "intf/stdlib.h"

DEFINE_DBG_ASSERT_I (memory_pager, memory_pager, p)
{
  ASSERT (p);
  ASSERT (p->idx < MEMORY_PAGE_LEN);
}

void
mpgr_create (memory_pager *dest)
{
  ASSERT (dest);
  dest->idx = 0;

  // Sets every is_present to false
  i_memset (dest->pages, 0, sizeof (dest->pages));
}

page *
mpgr_new (memory_pager *p, pgno pgno)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      page_wrapper *mp = &p->pages[p->idx];
      p->idx = (p->idx + 1) % MEMORY_PAGE_LEN;

      if (!mp->is_present)
        {
          mp->page.pg = pgno;
          mp->page.type = PG_UNKNOWN;

          mp->is_present = true;
          return &mp->page;
        }
    }

  return NULL;
}

bool
mpgr_is_full (const memory_pager *p)
{
  memory_pager_assert (p);

  u32 idx = p->idx;

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      const page_wrapper *mp = &p->pages[idx];
      idx = (p->idx + 1) % MEMORY_PAGE_LEN;

      if (!mp->is_present)
        {
          return false;
        }
    }

  return true;
}

page *
mpgr_get_rw (memory_pager *p, u64 pgno)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      page_wrapper *mp = &p->pages[(i + p->idx) % MEMORY_PAGE_LEN];
      if (mp->is_present && mp->page.pg == pgno)
        {
          return &mp->page;
        }
    }

  return NULL;
}

const page *
mpgr_get_r (const memory_pager *p, u64 pgno)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      const page_wrapper *mp = &p->pages[(i + p->idx) % MEMORY_PAGE_LEN];
      if (mp->is_present && mp->page.pg == pgno)
        {
          return &mp->page;
        }
    }

  return NULL;
}

u64
mpgr_get_evictable (const memory_pager *p)
{
  memory_pager_assert (p);
  const page_wrapper *mp = &p->pages[p->idx];
  ASSERT (mp->is_present);
  return mp->page.pg;
}

bool
mpgr_get_next (pgno *dest, const memory_pager *p)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      const page_wrapper *mp = &p->pages[(i + p->idx) % MEMORY_PAGE_LEN];
      if (mp->is_present)
        {
          *dest = mp->page.pg;
          return true;
        }
    }

  return false;
}

void
mpgr_evict (memory_pager *p, u64 pgno)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {

      page_wrapper *mp = &p->pages[p->idx];

      if (mp->is_present && mp->page.pg == pgno)
        {
          mp->is_present = 0;
          return;
        }

      p->idx = (p->idx + 1) % MEMORY_PAGE_LEN;
    }
  UNREACHABLE ();
}
