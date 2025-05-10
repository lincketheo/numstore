#include "paging/pager.h"
#include "errors/error.h"
#include "paging/file_pager.h"
#include "paging/memory_pager.h"
#include "paging/page.h"

///////////////////////////// PAGER

DEFINE_DBG_ASSERT_I (pager, pager, p)
{
  ASSERT (p);
}

err_t
pgr_create (pager *dest, pgr_params params, error *e)
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

  err_t_wrap (mpgr_create (&mpgr, mparams, e), e);
  err_t_wrap (fpgr_create (&fpgr, fparams, e), e);

  dest->mpager = mpgr;
  dest->fpager = fpgr;
  dest->page_size = params.page_size;

  pager_assert (dest);

  return SUCCESS;
}

static inline err_t
pgr_evict (pager *p, error *e)
{
  // Find the next evictable target
  pgno evict = mpgr_get_evictable (&p->mpager);

  // Commit that page
  u8 *evict_page = mpgr_get (&p->mpager, evict);
  ASSERT (evict_page);

  err_t_wrap (pgr_commit (p, evict_page, evict, e), e);

  // Then evict that page
  mpgr_evict (&p->mpager, evict);
  return SUCCESS;
}

err_t
pgr_get_expect (page *dest, int type, pgno pgno, pager *p, error *e)
{
  pager_assert (p);
  u8 *raw = mpgr_get (&p->mpager, pgno);

  // Cache miss
  if (raw == NULL)
    {
      if (mpgr_is_full (&p->mpager))
        {
          err_t_wrap (pgr_evict (p, e), e);
        }
      ASSERT (!mpgr_is_full (&p->mpager));

      // Then create a new in memory page
      raw = mpgr_new (&p->mpager, pgno);
      ASSERT (raw); // Should be present

      // And read it into the actual page
      err_t_wrap (fpgr_get_expect (&p->fpager, raw, pgno, e), e);
    }

  return page_read_expect (dest, type, raw, p->page_size, pgno, e);
}

err_t
pgr_new (page *dest, pager *p, page_type type, error *e)
{
  ASSERT (dest);
  pager_assert (p);

  // Create new page in file system
  pgno pgno = 0;
  err_t_wrap (fpgr_new (&p->fpager, &pgno, e), e);

  // Create room in memory to hold it
  u8 *raw = mpgr_new (&p->mpager, pgno);
  if (raw == NULL)
    {
      // Evict a page if no room
      err_t_wrap (pgr_evict (p, e), e);

      // Try again
      raw = mpgr_new (&p->mpager, pgno);
      ASSERT (raw);
    }

  // And read it into the actual page
  err_t_wrap (fpgr_get_expect (&p->fpager, raw, pgno, e), e);

  page_init (dest, type, raw, p->page_size, pgno);

  return SUCCESS;
}

err_t
pgr_commit (pager *p, u8 *data, pgno pgno, error *e)
{
  pager_assert (p);

  err_t_wrap (fpgr_commit (&p->fpager, data, pgno, e), e);

  return SUCCESS;
}
