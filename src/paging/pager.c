#include "paging/pager.h"
#include "core/string.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "os/io.h"
#include "os/logging.h"
#include "os/stdlib.h"
#include "paging/file_pager.h"
#include "paging/memory_page.h"
#include "paging/memory_pager.h"

#ifndef NDEBUG
static u64 cache_hit = 0;
static u64 cache_miss = 0;

void
pgr_log_stats ()
{
  i_log_info ("Pager Stats:\n");
  i_log_info ("Cache hit rate: %f\n",
              (float)cache_hit / (float)(cache_hit + cache_miss));
}
#endif

DEFINE_DBG_ASSERT_I (pager, pager, p)
{
  ASSERT (p);
  memory_pager_assert (&p->mpager);
  file_pager_assert (&p->fpager);
}

int
pgr_new (pager *p, u64 *dest)
{
  pager_assert (p);
  ASSERT (dest);

  return fpgr_new (&p->fpager, dest);
}

int
pgr_get (pager *p, u8 **dest, u64 ptr)
{
  pager_assert (p);
  ASSERT (dest);

  u8 *page;

  // Check if it's in memory
  if (mpgr_check_page_exists (&p->mpager, ptr) >= 0)
    {

#ifndef NDEBUG
      cache_hit++;
#endif

      page = mpgr_get (&p->mpager, ptr);
      ASSERT (page);
      *dest = page;
      return SUCCESS;
    }
  else
    {

#ifndef NDEBUG
      cache_miss++;
#endif

      // Check if space is full
      if (mpgr_find_avail (&p->mpager) < 0)
        {
          u64 toevict = mpgr_get_evictable (&p->mpager);
          mpgr_delete (&p->mpager, toevict);
        }

      // Allocate new memory page
      page = mpgr_new (&p->mpager, ptr);
      ASSERT (page != NULL);

      // Retrieve page from disk
      int ret;
      if ((ret = fpgr_get (&p->fpager, page, ptr)))
        {
          return ret;
        }
      *dest = page;
      return SUCCESS;
    }
}

// Writes page to disk
int
pgr_commit (pager *p, u64 ptr)
{
  pager_assert (p);

  u8 *page = mpgr_get (&p->mpager, ptr);

  int ret = fpgr_commit (&p->fpager, page, ptr);
  if (ret)
    {
      i_log_warn ("Failed to commit page inside pager\n");
      return ret;
    }

  return SUCCESS;
}
