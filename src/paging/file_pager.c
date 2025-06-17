#include "paging/file_pager.h"

#include "config.h"      // PAGE_SIZE
#include "dev/testing.h" // TEST
#include "errors/error.h"
#include "intf/io.h"
#include "intf/stdlib.h" // i_memset

///////////////////////////// FILE PAGING

struct file_pager_s
{
  pgno npages; // Cached so you don't need to call fsize a ton
  i_file f;    // The file we're working with
};

DEFINE_DBG_ASSERT_I (file_pager, file_pager, p)
{
  ASSERT (p);
}

static const char *TAG = "File Pager";

/**
 * Sets the total file length of [p]
 * in pages
 */
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

file_pager *
fpgr_open (const string fname, error *e)
{
  file_pager *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (
          e,
          ERR_NOMEM,
          "Failed to allocate memory for file pager");
      return ret;
    }

  err_t_wrap_null (i_open_rw (&ret->f, fname, e), e);
  err_t_wrap_null (fpgr_set_len (ret, e), e);

  file_pager_assert (ret);

  return ret;
}

TEST (fpgr_open)
{
  _Static_assert(PAGE_SIZE > 2, "PAGE_SIZE should be > 2 for file_pager test");

  // The temp file name
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  error e = error_create (NULL);
  test_fail_if (i_mkstemp (&fp, tmpl, &e));

  // edge case: file shorter than header
  test_fail_if (i_truncate (&fp, PAGE_SIZE - 1, &e));
  file_pager *pager = fpgr_open (tmpl, &e);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  test_assert_equal (pager, NULL);
  e.cause_code = SUCCESS;

  // edge case: file size = half a page
  test_fail_if (i_truncate (&fp, PAGE_SIZE / 2, &e));
  pager = fpgr_open (tmpl, &e);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  test_assert_equal (pager, NULL);
  e.cause_code = SUCCESS;

  // happy path: file exactly header size, zero pages
  test_fail_if (i_truncate (&fp, 0, &e));
  pager = fpgr_open (tmpl, &e);
  test_assert_int_equal (e.cause_code, SUCCESS);
  test_fail_if_null (pager);
  test_assert_int_equal ((int)pager->npages, 0);
  test_fail_if (fpgr_close (pager, &e));

  // happy path: file exactly header size, more pages
  test_fail_if (i_truncate (&fp, 3 * PAGE_SIZE, &e));
  pager = fpgr_open (tmpl, &e);
  test_assert_int_equal (e.cause_code, SUCCESS);
  test_assert_equal (pager->npages, 3);
  test_fail_if (fpgr_close (pager, &e));

  // There were 2 references to file - close it here too
  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink (tmpl, &e));
}

err_t
fpgr_close (file_pager *f, error *e)
{
  file_pager_assert (f);

  // Close the file
  err_t_wrap (i_close (&f->f, e), e);

  // Free the memory
  i_free (f);

  return SUCCESS;
}

p_size
fpgr_get_npages (const file_pager *fp)
{
  file_pager_assert (fp);
  return fp->npages;
}

err_t
fpgr_new (file_pager *p, u64 *dest, error *e)
{
  file_pager_assert (p);
  ASSERT (dest);

  // Extend the file by 1 page
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

  test_fail_if (i_truncate (&fp, 0, &e));

  file_pager *p = fpgr_open (tmpl, &e);
  test_assert_int_equal (e.cause_code, SUCCESS);
  test_fail_if_null (p);

  u64 pgno;

  // Create a new page
  test_fail_if (fpgr_new (p, &pgno, &e));

  // Page should be at position 0
  test_assert_int_equal (pgno, 0);

  // There should be 1 page
  test_assert_int_equal (p->npages, 1);

  // Size should be PAGE_SIZE
  test_assert_int_equal (i_file_size (&fp, &e), PAGE_SIZE * p->npages);

  // Add two more pages and do the same thing
  test_fail_if (fpgr_new (p, &pgno, &e));
  test_assert_int_equal (pgno, 1);
  test_assert_int_equal (p->npages, 2);
  test_assert_int_equal (i_file_size (&fp, &e), PAGE_SIZE * p->npages);

  test_fail_if (fpgr_new (p, &pgno, &e));
  test_assert_int_equal (pgno, 2);
  test_assert_int_equal (p->npages, 3);
  test_assert_int_equal (i_file_size (&fp, &e), PAGE_SIZE * p->npages);

  test_fail_if (fpgr_close (p, &e));

  // There were 2 references to file - close it here too
  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink (tmpl, &e));
}

err_t
fpgr_read (file_pager *p, u8 dest[PAGE_SIZE], u64 pgno, error *e)
{
  file_pager_assert (p);
  ASSERT (dest);
  if (pgno > p->npages)
    {
      return error_causef (e, ERR_CORRUPT, "File Pager: Invalid page index");
    }

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

TEST (fpgr_read_write)
{
  // The raw page bytes
  u8 _page[PAGE_SIZE];

  // Create a temporary file
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  error e = error_create (NULL);
  test_fail_if (i_mkstemp (&fp, tmpl, &e));

  // File should be size 0
  test_fail_if (i_truncate (&fp, 0, &e));

  // Open a new pager
  file_pager *p = fpgr_open (tmpl, &e);
  test_assert_int_equal (e.cause_code, SUCCESS);
  test_fail_if_null (p);

  // happy path: new page, write, then read back
  u64 pgno;
  test_fail_if (fpgr_new (p, &pgno, &e));

  // Write 0 : PAGE_SIZE to each byte (overflow fine, it's just data)
  for (u32 i = 0; i < PAGE_SIZE; i++)
    {
      _page[i] = (u8)i;
    }
  // Write it out
  test_fail_if (fpgr_write (p, _page, pgno, &e));

  // Scramble page so we can read it back in
  i_memset (_page, 0xFF, PAGE_SIZE);
  test_fail_if (fpgr_read (p, _page, pgno, &e));

  // Iterate and check that it matches what we expect
  for (u32 i = 0; i < PAGE_SIZE; i++)
    {
      test_assert_int_equal (_page[i], (u8)i);
    }

  // There's 2 references to this file, close the other one
  test_fail_if (fpgr_close (p, &e));
  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink (tmpl, &e));
}
