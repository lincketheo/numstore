#include "paging/pager.h"

#include "errors/error.h"
#include "intf/io.h" // i_malloc
#include "paging/file_pager.h"
#include "paging/memory_pager.h"

///////////////////////////// PAGER

DEFINE_DBG_ASSERT_I (pager, pager, p)
{
  ASSERT (p);
}

static const char *TAG = "Pager";

pager *
pgr_create (const string fname, error *e)
{
  if (!i_exists_rw (fname))
    {
      error_causef (
          e, ERR_DOESNT_EXIST,
          "Database file: %.*s doesn't exist",
          fname.len, fname.data);
      return NULL;
    }

  pager *ret = i_calloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "%s: Failed to allocate pager", TAG);
      return NULL;
    }

  if (fpgr_create (&ret->fpager, fname, e))
    {
      i_free (ret);
      return NULL;
    }

  mpgr_create (&ret->mpager);

  pager_assert (ret);

  return ret;
}

void
pgr_close (pager *p)
{
  pager_assert (p);
  fpgr_close (&p->fpager);
  i_free (p);
}

static inline err_t
pgr_evict (pager *p, error *e)
{
  // Find the next evictable target
  pgno evict = mpgr_get_evictable (&p->mpager);

  // Commit that page
  const page *evict_page = mpgr_get_r (&p->mpager, evict);
  ASSERT (evict_page);

  err_t_wrap (pgr_write (p, evict_page, e), e);

  // Then evict that page
  mpgr_evict (&p->mpager, evict);
  return SUCCESS;
}

page *
pgr_get_expect_rw (int type, pgno pgno, pager *p, error *e)
{
  pager_assert (p);
  page *pg = mpgr_get_rw (&p->mpager, pgno);

  // Cache miss
  if (pg == NULL)
    {
      if (mpgr_is_full (&p->mpager) && pgr_evict (p, e))
        {
          return NULL;
        }
      ASSERT (!mpgr_is_full (&p->mpager));

      // Then create a new in memory page
      pg = mpgr_new (&p->mpager, pgno);
      ASSERT (pg); // Should be present

      // And read it into the actual page
      if (fpgr_get_expect (&p->fpager, pg->raw, pgno, e))
        {
          return NULL;
        }
    }

  if (page_set_ptrs_expect_type (pg, type, e))
    {
      return NULL;
    }

  return pg;
}

const page *
pgr_get_expect_r (int type, pgno pgno, pager *p, error *e)
{
  return pgr_get_expect_rw (type, pgno, p, e);
}

page *
pgr_new (pager *p, page_type type, error *e)
{
  pager_assert (p);

  // Create new page in file system
  pgno pgno = 0;
  if (fpgr_new (&p->fpager, &pgno, e))
    {
      return NULL;
    }

  // Create room in memory to hold it
  page *pg = mpgr_new (&p->mpager, pgno);
  if (pg == NULL)
    {
      // Evict a page if no room
      if (pgr_evict (p, e))
        {
          return NULL;
        }

      // Try again
      pg = mpgr_new (&p->mpager, pgno);
      ASSERT (pg);
    }

  // And read it into the actual page
  if (fpgr_get_expect (&p->fpager, pg->raw, pgno, e))
    {
      return NULL;
    }

  page_init (pg, type);

  return pg;
}

err_t
pgr_write (pager *p, const page *pg, error *e)
{
  pager_assert (p);

  err_t_wrap (fpgr_write (&p->fpager, pg->raw, pg->pg, e), e);

  return SUCCESS;
}
