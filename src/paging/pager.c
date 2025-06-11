#include "paging/pager.h"

#include "config.h"
#include "dev/testing.h"
#include "ds/robin_hood_ht.h"
#include "errors/error.h"
#include "intf/io.h" // i_malloc
#include "intf/logging.h"
#include "intf/types.h"
#include "paging/file_pager.h"
#include "paging/page.h"

///////////////////////////// PAGER

typedef struct
{
  page page;
  int access_bit;
  bool present;
} page_wrapper;

struct pager_s
{
  file_pager *fp;

  hash_table *pgno_to_index;
  page_wrapper pages[MEMORY_PAGE_LEN];
  u32 clock;
};

DEFINE_DBG_ASSERT_I (pager, pager, p)
{
  ASSERT (p);
  ASSERT (p->fp);
  ASSERT (p->pgno_to_index);
  ASSERT (p->clock < MEMORY_PAGE_LEN);
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

  hash_table *pgno_to_index = ht_open (MEMORY_PAGE_LEN, e);
  if (pgno_to_index == NULL)
    {
      i_free (ret);
      err_t_log_swallow (fpgr_close (fpgr, &_e), _e);
      return NULL;
    }

  ret->pgno_to_index = pgno_to_index;
  ret->fp = fpgr;
  ret->clock = 0;

  pager_assert (ret);

  return ret;
}

// Returns the last error
static err_t
pgr_save_all (pager *pg, error *e)
{
  err_t ret = SUCCESS;

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      page_wrapper *mp = &pg->pages[pg->clock];
      pg->clock = (pg->clock + 1) % MEMORY_PAGE_LEN;

      if (mp->present)
        {
          // Write data out
          err_t _ret = pgr_save (pg, &mp->page, e);
          if (_ret != SUCCESS)
            {
              ret = _ret;
              error_log_consume (e);
            }

          // Delete from in memory pool
          mp->present = false;

          // Delete from hash table
          hta_res r = ht_delete (pg->pgno_to_index, mp->page.pg);
          ASSERT (r == HTAR_SUCCESS);
        }
    }

  return ret;
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

// Reserves a spot at clock by evicting pages
static inline err_t
pgr_reserve (pager *p, error *e)
{
  pager_assert (p);

  // Find a spot to fit it in memory
  u32 i = 0;
  for (; i < MEMORY_PAGE_LEN; ++i)
    {
      page_wrapper *mp = &p->pages[p->clock];

      // Evict
      if (mp->present && mp->access_bit == 0)
        {
          // Write data out
          err_t_wrap (fpgr_write (p->fp, mp->page.raw, mp->page.pg, e), e);

          // Delete from in memory pool
          mp->present = false;

          // Delete from hash table
          hta_res r = ht_delete (p->pgno_to_index, mp->page.pg);
          ASSERT (r == HTAR_SUCCESS);

          // Use this newly opened spot
          break;
        }

      // Don't evict yet
      else if (mp->present)
        {
          // Update access bit and continue searching
          mp->access_bit = 0;
          p->clock = (p->clock + 1) % MEMORY_PAGE_LEN;
        }

      // Otherwise, empty, just use this spot
      else
        {
          break;
        }
    }

  // Check for full condition
  ASSERT (i <= MEMORY_PAGE_LEN);
  if (i == MEMORY_PAGE_LEN)
    {
      return error_causef (
          e, ERR_NOMEM, "%s Memory buffer pool is full", TAG);
    }

  // We reserved room at clock room
  ASSERT (!p->pages[p->clock].present);

  return SUCCESS;
}

static page_wrapper *
pgr_load (pager *p, pgno pg, error *e)
{
  pager_assert (p);

  // Try fetching it from memory first
  hdata data;
  switch (ht_get (p->pgno_to_index, &data, pg))
    {
    case HTAR_SUCCESS:
      {
        return &p->pages[data.index];
      }
    case HTAR_DOESNT_EXIST:
      {
        // Need to do some more work
        break;
      }
    }

  // Reserve room for the new page to be read in
  err_t_wrap_null (pgr_reserve (p, e), e);

  // Read in data to the current page
  page_wrapper *mp = &p->pages[p->clock];
  err_t_wrap_null (fpgr_read (p->fp, mp->page.raw, pg, e), e);

  mp->access_bit = 0;
  mp->present = true;
  mp->page.pg = pg;

  // Insert into the hash table
  hti_res res = ht_insert (
      p->pgno_to_index,
      (hdata){
          .key = pg,
          .index = p->clock,
      });
  ASSERT (res == HTIR_SUCCESS);

  // Rotate circular index (like a stack)
  p->clock = (p->clock + 1) % MEMORY_PAGE_LEN;

  return mp;
}

const page *
pgr_get (int type, pgno pg, pager *p, error *e)
{
  page_wrapper *ret = pgr_load (p, pg, e);
  if (ret == NULL)
    {
      return NULL;
    }
  ret->access_bit = 1;
  err_t_wrap_null (page_set_ptrs_expect_type (&ret->page, type, e), e);

  return &ret->page;
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

  page_wrapper *ret = pgr_load (p, pg, e);
  if (ret == NULL)
    {
      return NULL;
    }

  page_init (&ret->page, type);

  return &ret->page;
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

  i_log_trace ("Saving page: %" PRpgno "\n", pg->pg);
  err_t_wrap (fpgr_write (p->fp, pg->raw, pg->pg, e), e);

  return SUCCESS;
}

TEST (pgr_open_basic)
{
  char _tmpl[] = "/tmp/pgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  error e = error_create (NULL);
  test_fail_if (i_mkstemp (&fp, tmpl, &e));
  pager *p;

  // File is shorter than page size
  test_fail_if (i_truncate (&fp, PAGE_SIZE - 1, &e));
  p = pgr_open (tmpl, &e);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  test_assert_equal (p, NULL);
  e.cause_code = SUCCESS;

  // Half a page
  test_fail_if (i_truncate (&fp, PAGE_SIZE / 2, &e));
  p = pgr_open (tmpl, &e);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  test_assert_equal (p, NULL);
  e.cause_code = SUCCESS;

  // 0 pages
  test_fail_if (i_truncate (&fp, 0, &e));
  p = pgr_open (tmpl, &e);
  test_assert_int_equal (e.cause_code, SUCCESS);
  test_fail_if_null (p);
  test_assert_int_equal ((int)pgr_get_npages (p), 0);
  test_fail_if (pgr_close (p, &e));

  // 3 pages
  test_fail_if (i_truncate (&fp, 3 * PAGE_SIZE, &e));
  p = pgr_open (tmpl, &e);
  test_assert_int_equal (e.cause_code, SUCCESS);
  test_fail_if_null (p);
  test_assert_int_equal ((int)pgr_get_npages (p), 3);
  test_fail_if (pgr_close (p, &e));

  // Tear down
  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink (tmpl, &e));
}

TEST (pgr_new_get_save)
{
  char _tmpl[] = "/tmp/pgr_ngsXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  error e = error_create (NULL);
  test_fail_if (i_mkstemp (&fp, tmpl, &e));

  pager *p = pgr_open (tmpl, &e);
  test_fail_if_null (p);

  const page *pg = pgr_new (p, PG_DATA_LIST, &e);
  test_fail_if_null (pg);
  test_assert_int_equal ((int)pgr_get_npages (p), 1);

  const page *pg2 = pgr_get (PG_DATA_LIST, 0, p, &e);
  test_fail_if_null (pg2);

  page *w = pgr_make_writable (p, pg2);
  test_fail_if_null (w);
  ((u8 *)w->dl.data)[0] = 0xAA;
  test_fail_if (pgr_save (p, w, &e));

  test_fail_if (pgr_close (p, &e));

  // Tear down
  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink (tmpl, &e));
}

TEST (pgr_corrupt_fetch)
{
  char _tmpl[] = "/tmp/pgr_corruptXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  error e = error_create (NULL);
  test_fail_if (i_mkstemp (&fp, tmpl, &e));

  pager *p = pgr_open (tmpl, &e);
  test_fail_if_null (p);

  // Non-existent page
  const page *bad = pgr_get (PG_DATA_LIST, 42, p, &e);
  test_assert_equal (bad, NULL);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  e.cause_code = SUCCESS;

  // New page
  const page *pg = pgr_new (p, PG_DATA_LIST, &e);
  test_fail_if_null (pg);

  // Get wrong type
  const page *bad2 = pgr_get (PG_INNER_NODE, 0, p, &e);
  test_assert_equal (bad2, NULL);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  e.cause_code = SUCCESS;

  // Tear down
  test_fail_if (pgr_close (p, &e));
  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink (tmpl, &e));
}
