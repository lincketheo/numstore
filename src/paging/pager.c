#include "paging/pager.h"

#include "errors/error.h"
#include "intf/io.h" // i_malloc
#include "paging/file_pager.h"
#include "paging/memory_pager.h"

///////////////////////////// PAGER

struct pager_s
{
  file_pager *fp;
  memory_pager *mp;
};

DEFINE_DBG_ASSERT_I (pager, pager, p)
{
  ASSERT (p);
  ASSERT (p->fp);
  ASSERT (p->mp);
}

static const char *TAG = "Pager";

pager *
pgr_open (const string fname, error *e)
{
  // Allocate pager
  pager *ret = i_calloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "%s: Failed to allocate pager", TAG);
      return NULL;
    }

  // Create the file pager
  file_pager *fpgr = fpgr_open (fname, e);
  if (fpgr == NULL)
    {
      i_free (ret);
      return NULL;
    }

  // Create the memory pager
  memory_pager *mpgr = mpgr_open (e);
  if (mpgr == NULL)
    {
      i_free (ret);
      err_t_log_swallow (fpgr_close (fpgr, &_e), _e);
      return NULL;
    }

  ret->fp = fpgr;
  ret->mp = mpgr;

  pager_assert (ret);

  return ret;
}

// Returns the last error
static err_t
pgr_save_all (pager *pg, error *e)
{
  const page *cur = mpgr_pop (pg->mp);
  const page *next;

  if (cur == NULL)
    {
      return SUCCESS;
    }

  next = mpgr_pop (pg->mp);

  while (cur != NULL)
    {
      if (fpgr_write (pg->fp, cur->raw, cur->pg, e))
        {
          if (next != NULL)
            {
              error_log_consume (e);
            }
        }

      cur = next;
      next = mpgr_pop (pg->mp);
    }

  return e->cause_code;
}

err_t
pgr_close (pager *p, error *e)
{
  // manage errors
  pager_assert (p);

  // Save all in memory pages
  err_t_wrap (pgr_save_all (p, e), e);

  // Close resources
  err_t_wrap (fpgr_close (p->fp, e), e);
  mpgr_close (p->mp);

  // Free resources
  i_free (p);

  return err_t_from (e);
}

p_size
pgr_get_npages (const pager *p)
{
  pager_assert (p);
  return fpgr_get_npages (p->fp);
}

const page *
pgr_get (int type, pgno pg, pager *p, error *e)
{
  pager_assert (p);
  spgno evictable;
  page *epg = mpgr_get (p->mp, pg, &evictable);

  // Cache miss
  if (epg == NULL)
    {
      if (evictable >= 0)
        {
          mpgr_evict (p->mp, evictable);
        }

      // Then create a new in memory page
      epg = mpgr_new (p->mp, pg, NULL);
      ASSERT (pg); // Should be present

      // And read it into the actual page
      if (fpgr_read (p->fp, epg->raw, pg, e))
        {
          return NULL;
        }
    }

  if (page_set_ptrs_expect_type (epg, type, e))
    {
      return NULL;
    }

  return epg;
}

const page *
pgr_new (pager *p, page_type type, error *e)
{
  pager_assert (p);

  // Create new page in file system
  pgno pg = 0;
  if (fpgr_new (p->fp, &pg, e))
    {
      return NULL;
    }

  // Create room in memory to hold it
  pgno evictable;
  page *pge = mpgr_new (p->mp, pg, &evictable);
  if (pge == NULL)
    {
      // Evict a page if no room
      const page *epg = mpgr_get (p->mp, evictable, NULL);
      err_t_wrap_null (pgr_save (p, epg, e), e);

      // Then evict that page
      mpgr_evict (p->mp, evictable);

      // Try again
      pge = mpgr_new (p->mp, pg, NULL);
      ASSERT (pg);
    }

  // And read it into the actual page
  if (fpgr_read (p->fp, pge->raw, pg, e))
    {
      return NULL;
    }

  // Initialize that page
  page_init (pge, type);

  return pge;
}

page *
pgr_make_writable (pager *p, const page *pg)
{
  pager_assert (p);
  // TODO
  return (page *)pg;
}

err_t
pgr_save (pager *p, const page *pg, error *e)
{
  pager_assert (p);

  err_t_wrap (fpgr_write (p->fp, pg->raw, pg->pg, e), e);

  return SUCCESS;
}
