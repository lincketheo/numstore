#include "paging/file_pager.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/stdlib.h"

///////////////////////////// FILE PAGING

DEFINE_DBG_ASSERT_I (file_pager, file_pager, p)
{
  ASSERT (p);
}

static inline err_t
fpgr_set_len (file_pager *p)
{
  file_pager_assert (p);

  i64 size = i_file_size (&p->f);

  // File must be at least header size
  if (size < p->header_size)
    {
      return ERR_INVALID_STATE;
    }

  // Remaining file must be a multiple of page_size
  if ((size - p->header_size) % p->page_size != 0)
    {
      return ERR_INVALID_STATE;
    }

  p->npages = (size - p->header_size) / p->page_size;
  return SUCCESS;
}

err_t
fpgr_create (file_pager *dest, fpgr_params params)
{
  ASSERT (dest);

  dest->header_size = params.header_size;
  dest->f = params.f;
  dest->page_size = params.page_size;

  err_t ret = fpgr_set_len (dest);
  if (ret)
    {
      return ret;
    }

  file_pager_assert (dest);

  return SUCCESS;
}

TEST (fpgr_create)
{
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  test_fail_if (i_mkstemp (&fp, tmpl));
  test_fail_if (i_unlink (tmpl));
  file_pager pager;

  // edge: file shorter than header
  test_fail_if (i_truncate (&fp, 9));
  test_assert_int_equal (fpgr_create (
                             &pager, (fpgr_params){
                                         .f = fp,
                                         .page_size = 2048,
                                         .header_size = 10 }),
                         ERR_INVALID_STATE);

  // edge: file size = header + half a page
  test_fail_if (i_truncate (&fp, 10 + 2048 / 2));
  test_assert_int_equal (fpgr_create (
                             &pager, (fpgr_params){
                                         .f = fp,
                                         .page_size = 2048,
                                         .header_size = 10 }),
                         ERR_INVALID_STATE);

  // happy: file exactly header size, zero pages
  test_fail_if (i_truncate (&fp, 10));
  test_assert_int_equal (fpgr_create (
                             &pager, (fpgr_params){
                                         .f = fp,
                                         .page_size = 2048,
                                         .header_size = 10 }),
                         SUCCESS);
  test_assert_int_equal (pager.header_size, 10);
  test_assert_int_equal (pager.page_size, 2048);
  test_assert_int_equal ((int)pager.npages, 0);

  // happy: file exactly header size, more pages
  test_fail_if (i_truncate (&fp, 10 + 3 * 2048));
  test_assert_int_equal (fpgr_create (
                             &pager, (fpgr_params){
                                         .f = fp,
                                         .page_size = 2048,
                                         .header_size = 10 }),
                         SUCCESS);
  test_assert_int_equal (pager.header_size, 10);
  test_assert_int_equal (pager.page_size, 2048);
  test_assert_int_equal ((int)pager.npages, 3);

  test_fail_if (i_close (&fp));
}

err_t
fpgr_new (file_pager *p, u64 *dest)
{
  file_pager_assert (p);
  ASSERT (dest);

  u64 newsize = p->header_size + (p->page_size) * (p->npages + 1);
  err_t ret = i_truncate (&p->f, newsize);
  if (ret)
    {
      return ret;
    }

  *dest = p->npages++;

  return SUCCESS;
}

TEST (fpgr_new)
{
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  test_fail_if (i_mkstemp (&fp, tmpl));
  test_fail_if (i_unlink (tmpl));

  test_fail_if (i_truncate (&fp, 10));

  file_pager p;
  test_assert_int_equal (fpgr_create (
                             &p, (fpgr_params){
                                     .f = fp,
                                     .page_size = 2048,
                                     .header_size = 10 }),
                         SUCCESS);

  u64 pgno;

  test_fail_if (fpgr_new (&p, &pgno));
  test_assert_int_equal ((int)pgno, 0);
  test_assert_int_equal ((int)p.npages, 1);
  test_assert_int_equal ((int)i_file_size (&fp),
                         (int)(p.header_size + p.page_size * p.npages));

  test_fail_if (fpgr_new (&p, &pgno));
  test_assert_int_equal ((int)pgno, 1);
  test_assert_int_equal ((int)p.npages, 2);
  test_assert_int_equal ((int)i_file_size (&fp),
                         (int)(p.header_size + p.page_size * p.npages));

  test_fail_if (fpgr_new (&p, &pgno));
  test_assert_int_equal ((int)pgno, 2);
  test_assert_int_equal ((int)p.npages, 3);
  test_assert_int_equal ((int)i_file_size (&fp),
                         (int)(p.header_size + p.page_size * p.npages));

  test_fail_if (i_close (&fp));
}

err_t
fpgr_get_expect (file_pager *p, u8 *dest, u64 pgno)
{
  file_pager_assert (p);
  ASSERT (dest);
  ASSERT (pgno < p->npages);

  // Read all from file
  i64 nread = i_pread_all (
      &p->f, dest,
      p->page_size,
      p->header_size + pgno * p->page_size);

  if (nread == 0)
    {
      return ERR_INVALID_STATE;
    }

  if (nread < 0)
    {
      return ERR_IO;
    }

  return SUCCESS;
}

TEST (fpgr_get_expect)
{
  u8 _page[2048];

  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  test_fail_if (i_mkstemp (&fp, tmpl));
  test_fail_if (i_unlink (tmpl));

  test_fail_if (i_truncate (&fp, 10));

  file_pager p;
  test_assert_int_equal (fpgr_create (
                             &p, (fpgr_params){
                                     .f = fp,
                                     .page_size = 2048,
                                     .header_size = 10 }),
                         SUCCESS);

  // happy path: new page, write, then read back
  u64 pgno;
  test_fail_if (fpgr_new (&p, &pgno));

  u32 ps = p.page_size;
  for (u32 i = 0; i < ps; i++)
    {
      _page[i] = (u8)i;
    }
  test_fail_if (fpgr_commit (&p, _page, pgno));

  i_memset (_page, 0xFF, ps);
  test_fail_if (fpgr_get_expect (&p, _page, pgno));

  for (u32 i = 0; i < ps; i++)
    {
      test_assert_int_equal (_page[i], (u8)i);
    }

  i_close (&fp);
}

err_t
fpgr_delete (file_pager *p, u64 pgno __attribute__ ((unused)))
{
  // TODO
  file_pager_assert (p);
  return SUCCESS;
}

TEST (fpgr_delete)
{
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  test_fail_if (i_mkstemp (&fp, tmpl));
  test_fail_if (i_unlink (tmpl));

  test_fail_if (i_truncate (&fp, 10));

  file_pager p;
  test_assert_int_equal (fpgr_create (
                             &p, (fpgr_params){
                                     .f = fp,
                                     .page_size = 2048,
                                     .header_size = 10 }),
                         SUCCESS);

  // allocate two pages
  u64 a, b;
  test_fail_if (fpgr_new (&p, &a));
  test_fail_if (fpgr_new (&p, &b));

  // delete valid and out‑of‑range page numbers
  test_fail_if (fpgr_delete (&p, a));
  test_fail_if (fpgr_delete (&p, b + 5));

  test_fail_if (i_close (&fp));
}

err_t
fpgr_commit (file_pager *p, const u8 *src, u64 pgno)
{
  file_pager_assert (p);
  ASSERT (src);
  ASSERT (pgno < p->npages);

  // TODO - Figure out when to call invalid_state over err_io
  err_t ret = i_pwrite_all (
      &p->f, src,
      p->page_size,
      p->header_size + pgno * p->page_size);
  if (ret)
    {
      return ret;
    }

  return SUCCESS;
}

TEST (fpgr_commit)
{
  u8 _page[2048];

  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  test_fail_if (i_mkstemp (&fp, tmpl));
  test_fail_if (i_unlink (tmpl));

  test_fail_if (i_truncate (&fp, 10));

  file_pager p;
  test_assert_int_equal (fpgr_create (
                             &p, (fpgr_params){
                                     .f = fp,
                                     .page_size = 2048,
                                     .header_size = 10 }),
                         SUCCESS);

  // allocate one page
  u64 pgno;
  test_fail_if (fpgr_new (&p, &pgno));
  test_assert_int_equal ((int)i_file_size (&fp),
                         (int)(p.header_size + p.page_size * p.npages));

  i_memset (_page, 0xAB, p.page_size);

  // happy: writing to new page
  test_fail_if (fpgr_commit (&p, _page, pgno));

  test_fail_if (i_close (&fp));
}
