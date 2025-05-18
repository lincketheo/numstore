#include "paging/file_pager.h"

#include "config.h"      // PAGE_SIZE
#include "dev/testing.h" // TEST
#include "errors/error.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/stdlib.h" // i_memset

///////////////////////////// FILE PAGING

DEFINE_DBG_ASSERT_I (file_pager, file_pager, p)
{
  ASSERT (p);
}

static const char *TAG = "File Pager";

static inline err_t
fpgr_set_len (file_pager *p, error *e)
{
  file_pager_assert (p);

  i64 size = i_file_size (&p->f, e);
  if (size < 0)
    {
      return err_t_from (e);
    }

  if (size % PAGE_SIZE != 0)
    {
      return error_causef (
          e, ERR_CORRUPT,
          "%s: Database file size should be a multiple of "
          "PAGE_SIZE: %" PRp_size "actual file size was: %" PRId64,
          TAG, PAGE_SIZE, size);
    }

  p->npages = size / PAGE_SIZE;
  return SUCCESS;
}

err_t
fpgr_create (file_pager *dest, const string fname, error *e)
{
  ASSERT (dest);

  err_t_wrap (i_open_rw (&dest->f, fname, e), e);
  err_t_wrap (fpgr_set_len (dest, e), e);

  file_pager_assert (dest);

  return SUCCESS;
}

TEST (fpgr_create)
{
  _Static_assert(PAGE_SIZE > 2, "PAGE_SIZE should be > 2");

  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  error e = error_create (NULL);
  test_fail_if (i_mkstemp (&fp, tmpl, &e));
  file_pager pager;

  // edge: file shorter than header
  test_fail_if (i_truncate (&fp, PAGE_SIZE - 1, &e));
  test_assert_int_equal (fpgr_create (&pager, tmpl, &e), ERR_CORRUPT);
  e.cause_code = SUCCESS;

  // edge: file size = header + half a page
  test_fail_if (i_truncate (&fp, PAGE_SIZE / 2, &e));
  test_assert_int_equal (fpgr_create (&pager, tmpl, &e), ERR_CORRUPT);
  e.cause_code = SUCCESS;

  // happy: file exactly header size, zero pages
  test_fail_if (i_truncate (&fp, 0, &e));
  test_assert_int_equal (fpgr_create (&pager, tmpl, &e), SUCCESS);
  test_assert_int_equal ((int)pager.npages, 0);

  // happy: file exactly header size, more pages
  test_fail_if (i_truncate (&fp, 3 * PAGE_SIZE, &e));
  test_assert_int_equal (fpgr_create (&pager, tmpl, &e), SUCCESS);
  test_assert_int_equal ((int)pager.npages, 3);

  test_fail_if (i_close (&fp, &e));
}

void
fpgr_close (file_pager *f)
{
  file_pager_assert (f);
  error e = error_create (NULL);
  if (i_close (&f->f, &e))
    {
      i_log_warn ("Error closing file pager:\n");
      error_log_consume (&e);
    }
}

err_t
fpgr_new (file_pager *p, u64 *dest, error *e)
{
  file_pager_assert (p);
  ASSERT (dest);

  err_t_wrap (i_truncate (&p->f, PAGE_SIZE * (1 + p->npages), e), e);

  *dest = p->npages++;

  return SUCCESS;
}

TEST (fpgr_new)
{
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  error e = error_create (NULL);
  test_fail_if (i_mkstemp (&fp, tmpl, &e));

  test_fail_if (i_truncate (&fp, 10, &e));

  file_pager p;
  test_assert_int_equal (fpgr_create (&p, tmpl, &e), SUCCESS);

  u64 pgno;

  test_fail_if (fpgr_new (&p, &pgno, &e));
  test_assert_int_equal (pgno, 0);
  test_assert_int_equal (p.npages, 1);
  test_assert_int_equal (i_file_size (&fp, &e), PAGE_SIZE * p.npages);

  test_fail_if (fpgr_new (&p, &pgno, &e));
  test_assert_int_equal (pgno, 1);
  test_assert_int_equal (p.npages, 2);
  test_assert_int_equal (i_file_size (&fp, &e), PAGE_SIZE + p.npages);

  test_fail_if (fpgr_new (&p, &pgno, &e));
  test_assert_int_equal (pgno, 2);
  test_assert_int_equal (p.npages, 3);
  test_assert_int_equal (i_file_size (&fp, &e), PAGE_SIZE * p.npages);

  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink (tmpl, &e));
}

err_t
fpgr_get_expect (file_pager *p, u8 dest[PAGE_SIZE], u64 pgno, error *e)
{
  file_pager_assert (p);
  ASSERT (dest);
  ASSERT (pgno < p->npages);

  // Read all from file
  i64 nread = i_pread_all (&p->f, dest, PAGE_SIZE, pgno * PAGE_SIZE, e);

  if (nread == 0)
    {
      return error_causef (e, ERR_CORRUPT, "File pager: empty read");
    }

  if (nread < 0)
    {
      return err_t_from (e);
    }

  return SUCCESS;
}

TEST (fpgr_get_expect)
{
  u8 _page[2048];

  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  error e = error_create (NULL);
  test_fail_if (i_mkstemp (&fp, tmpl, &e));

  test_fail_if (i_truncate (&fp, 10, &e));

  file_pager p;
  test_assert_int_equal (fpgr_create (&p, tmpl, &e), SUCCESS);

  // happy path: new page, write, then read back
  u64 pgno;
  test_fail_if (fpgr_new (&p, &pgno, &e));

  for (u32 i = 0; i < PAGE_SIZE; i++)
    {
      _page[i] = (u8)i;
    }
  test_fail_if (fpgr_write (&p, _page, pgno, &e));

  i_memset (_page, 0xFF, PAGE_SIZE);
  test_fail_if (fpgr_get_expect (&p, _page, pgno, &e));

  for (u32 i = 0; i < PAGE_SIZE; i++)
    {
      test_assert_int_equal (_page[i], (u8)i);
    }

  i_close (&fp, &e);
  test_fail_if (i_unlink (tmpl, &e));
}

err_t
fpgr_delete (file_pager *p, u64 pgno __attribute__ ((unused)), error *e)
{
  // TODO
  (void)e;
  file_pager_assert (p);
  return SUCCESS;
}

err_t
fpgr_write (file_pager *p, const u8 src[PAGE_SIZE], u64 pgno, error *e)
{
  file_pager_assert (p);
  ASSERT (src);
  ASSERT (pgno < p->npages);

  err_t_wrap (i_pwrite_all (&p->f, src, PAGE_SIZE, pgno * PAGE_SIZE, e), e);

  return SUCCESS;
}

TEST (fpgr_write)
{
  u8 _page[2048];

  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  error e = error_create (NULL);
  test_fail_if (i_mkstemp (&fp, tmpl, &e));

  test_fail_if (i_truncate (&fp, 10, &e));

  file_pager p;
  test_assert_int_equal (fpgr_create (&p, tmpl, &e), SUCCESS);

  // allocate one page
  u64 pgno;
  test_fail_if (fpgr_new (&p, &pgno, &e));
  test_assert_int_equal (i_file_size (&fp, &e), PAGE_SIZE * p.npages);

  i_memset (_page, 0xAB, PAGE_SIZE);

  // happy: writing to new page
  test_fail_if (fpgr_write (&p, _page, pgno, &e));

  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink (tmpl, &e));
}
