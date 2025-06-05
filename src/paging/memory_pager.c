#include "paging/memory_pager.h"

#include "config.h"
#include "dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "dev/testing.h" // TEST
#include "errors/error.h"
#include "intf/io.h" // i_malloc
#include "intf/stdlib.h"

typedef struct
{
  page page;
  bool is_present;
} page_wrapper;

struct memory_pager_s
{
  page_wrapper pages[MEMORY_PAGE_LEN];
  u32 idx;
};

DEFINE_DBG_ASSERT_I (memory_pager, memory_pager, p)
{
  ASSERT (p);
  ASSERT (p->idx < MEMORY_PAGE_LEN);
}

memory_pager *
mpgr_open (error *e)
{
  memory_pager *ret = i_calloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "Failed to allocate memory for memory_pager");
      return ret;
    }

  ret->idx = 0;

  memory_pager_assert (ret);
  return ret;
}

void
mpgr_close (memory_pager *mp)
{
  memory_pager_assert (mp);
  i_free (mp);
}

static pgno
mpgr_get_evictable (const memory_pager *p)
{
  memory_pager_assert (p);
  const page_wrapper *mp = &p->pages[p->idx];
  ASSERT (mp->is_present);
  return mp->page.pg;
}

page *
mpgr_new (memory_pager *p, pgno pg, pgno *evictable)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      page_wrapper *mp = &p->pages[p->idx];
      p->idx = (p->idx + 1) % MEMORY_PAGE_LEN;

      if (!mp->is_present)
        {
          mp->page.pg = pg;
          mp->page.type = PG_UNKNOWN;

          mp->is_present = true;
          return &mp->page;
        }
    }

  // Set next evictable
  ASSERT (evictable);
  *evictable = mpgr_get_evictable (p);

  return NULL;
}

/**
 * Iterates through pages in memory_pager and removes is_present
 */
page *
mpgr_pop (memory_pager *p)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      page_wrapper *mp = &p->pages[(i + p->idx) % MEMORY_PAGE_LEN];
      if (mp->is_present)
        {
          mp->is_present = false;
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
mpgr_get (memory_pager *p, u64 pg, spgno *evictable)
{
  memory_pager_assert (p);

  bool room = true;

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      page_wrapper *mp = &p->pages[(i + p->idx) % MEMORY_PAGE_LEN];
      if (mp->is_present && mp->page.pg == pg)
        {
          return &mp->page;
        }
      else if (!mp->is_present)
        {
          room = true;
        }
    }

  ASSERT (evictable);
  if (room)
    {
      *evictable = -1;
    }
  else
    {
      *evictable = mpgr_get_evictable (p);
    }

  return NULL;
}

page *
mpgr_make_writable (const memory_pager *p, const page *pg)
{
  memory_pager_assert (p);
  ASSERT (pg);
  return (page *)pg; // TODO
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
