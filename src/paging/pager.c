#include "paging/pager.h"
#include "dev/errors.h"
#include "paging/file_pager.h"
#include "paging/memory_pager.h"
#include "paging/page.h"

///////////////////////////// PAGER

DEFINE_DBG_ASSERT_I (pager, pager, p)
{
  ASSERT (p);
}

err_t
pgr_create (pager *dest, pgr_params params)
{

  mpgr_params mparams = (mpgr_params){
    .page_size = params.page_size,
    .len = params.memory_pager_len,
    .alloc = params.memory_pager_allocator,
  };
  fpgr_params fparams = (fpgr_params){
    .page_size = params.page_size,
    .f = params.f,
    .header_size = params.header_size,
  };

  memory_pager mpgr;
  file_pager fpgr;

  err_t ret;

  if ((ret = mpgr_create (&mpgr, mparams)))
    {
      return ret;
    }
  if ((ret = fpgr_create (&fpgr, fparams)))
    {
      return ret;
    }

  dest->mpager = mpgr;
  dest->fpager = fpgr;
  dest->page_size = params.page_size;

  pager_assert (dest);

  return SUCCESS;
}

static inline err_t
pgr_evict (pager *p)
{
  // Find the next evictable target
  pgno evict = mpgr_get_evictable (&p->mpager);

  // Commit that page
  u8 *evict_page = mpgr_get (&p->mpager, evict);
  ASSERT (evict_page);

  err_t ret;
  if ((ret = pgr_commit (p, evict_page, evict)))
    {
      return ret;
    }

  // Then evict that page
  mpgr_evict (&p->mpager, evict);
  return SUCCESS;
}

err_t
pgr_get_expect (page *dest, int type, pgno pgno, pager *p)
{
  pager_assert (p);
  u8 *raw = mpgr_get (&p->mpager, pgno);
  err_t ret;

  // Cache miss
  if (raw == NULL)
    {
      // Check if we need to evict a page
      if (mpgr_is_full (&p->mpager))
        {
          // Evict a page
          if ((ret = pgr_evict (p)))
            {
              return ret;
            }
        }
      ASSERT (!mpgr_is_full (&p->mpager));

      // Then create a new in memory page
      raw = mpgr_new (&p->mpager, pgno);
      ASSERT (raw); // Should be present

      // And read it into the actual page
      if ((ret = fpgr_get_expect (&p->fpager, raw, pgno)))
        {
          return ret;
        }
    }

  return page_read_expect (dest, type, raw, p->page_size, pgno);
}

err_t
pgr_new (page *dest, pager *p, page_type type)
{
  ASSERT (dest);
  pager_assert (p);
  err_t ret;

  // Create new page in file system
  pgno pgno = 0;
  if ((ret = fpgr_new (&p->fpager, &pgno)))
    {
      return ret;
    }

  // Create room in memory to hold it
  u8 *raw = mpgr_new (&p->mpager, pgno);
  if (raw == NULL)
    {
      // Evict a page if no room
      if ((ret = pgr_evict (p)))
        {
          return ret;
        }

      // Try again
      raw = mpgr_new (&p->mpager, pgno);
      ASSERT (raw);
    }

  // And read it into the actual page
  if ((ret = fpgr_get_expect (&p->fpager, raw, pgno)))
    {
      return ret;
    }

  page_init (dest, type, raw, p->page_size, pgno);

  return SUCCESS;
}

err_t
pgr_commit (pager *p, u8 *data, pgno pgno)
{
  pager_assert (p);
  err_t ret;

  if ((ret = fpgr_commit (&p->fpager, data, pgno)))
    {
      return ret;
    }

  return SUCCESS;
}
