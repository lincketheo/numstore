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
  u64 evict = mpgr_get_evictable (&p->mpager);

  // Commit that page
  u8 *evict_page = mpgr_get (&p->mpager, evict);
  ASSERT (evict_page);
  err_t ret = pgr_commit (p, evict_page, evict);
  if (ret)
    {
      return ret;
    }

  // Then evict that page
  mpgr_evict (&p->mpager, evict);
  return SUCCESS;
}

err_t
pgr_get_expect (page *dest, page_type type, u64 pgno, pager *p)
{
  pager_assert (p);
  u8 *page = mpgr_get (&p->mpager, pgno);

  // Cache miss
  if (page == NULL)
    {
      err_t ret;
      // Check if we need to evict a page
      if (mpgr_is_full (&p->mpager))
        {

          // Evict a page
          ret = pgr_evict (p);
          if (ret)
            {
              return ret;
            }
        }
      ASSERT (!mpgr_is_full (&p->mpager));

      // Then create a new in memory page
      page = mpgr_new (&p->mpager, pgno);
      ASSERT (page); // Should be present

      // And read it into the actual page
      ret = fpgr_get_expect (&p->fpager, page, pgno);
      if (ret)
        {
          return ret;
        }
    }

  page_interpret_params params = (page_interpret_params){
    .page_size = p->page_size,
    .type = type,
    .raw = page,
    .pgno = pgno,
  };
  return page_read_expect (dest, params);
}

err_t
pgr_new (page *dest, pager *p, page_type type)
{
  ASSERT (dest);
  pager_assert (p);

  // Create new page in file system
  u64 pgno = 0;
  err_t ret = fpgr_new (&p->fpager, &pgno);
  if (ret)
    {
      return ret;
    }

  // Create room in memory to hold it
  u8 *raw = mpgr_new (&p->mpager, pgno);
  if (raw == NULL)
    {
      if ((ret = pgr_evict (p)))
        {
          return ret;
        }

      // Try again
      raw = mpgr_new (&p->mpager, pgno);
      ASSERT (raw);
    }

  // And read it into the actual page
  ret = fpgr_get_expect (&p->fpager, raw, pgno);
  if (ret)
    {
      return ret;
    }

  page_interpret_params params = (page_interpret_params){
    .page_size = p->page_size,
    .type = type,
    .raw = raw,
    .pgno = pgno,
  };
  page_init (dest, params);
  return SUCCESS;
}

err_t
pgr_commit (pager *p, u8 *data, u64 pgno)
{
  pager_assert (p);

  err_t ret = fpgr_commit (&p->fpager, data, pgno);
  if (ret)
    {
      return ret;
    }
  return SUCCESS;
}
