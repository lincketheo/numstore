#include "paging/pager.h"
#include "dev/errors.h"
#include "os/io.h"
#include "paging/file_pager.h"
#include "paging/memory_pager.h"

DEFINE_DBG_ASSERT (pager, pager, p)
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
      page = mpgr_get (&p->mpager, ptr);
      ASSERT (page);
      *dest = page;
      return SUCCESS;
    }
  else
    {
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
