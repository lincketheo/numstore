#include "paging.h"
#include "database.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "sds.h"
#include "types.h"

///////////////////////////// FILE PAGING

DEFINE_DBG_ASSERT_I (file_pager, file_pager, p)
{
  ASSERT (p);
  i_file_assert (p->f);
}

static inline err_t
fpgr_set_len (file_pager *p)
{
  file_pager_assert (p);
  i64 size = i_file_size (p->f); // TODO - can file size > i64?
  if (size < p->header_size)
    {
      i_log_warn ("Error encountered while trying to fetch "
                  "last page size in file_pager\n");
      return ERR_INVALID_STATE;
    }

  ASSERT (size >= p->header_size);
  if ((size - p->header_size) % p->page_size != 0)
    {
      i_log_error ("The database is in an invalid state "
                   "when trying to create a new page. File size "
                   "is not a multiple of page size: %" PRIu32 " bytes\n",
                   p->page_size);
      return ERR_INVALID_STATE;
    }

  ASSERT (size >= p->header_size);
  p->npages = (size - p->header_size) / p->page_size;
  return SUCCESS;
}

err_t
fpgr_create (file_pager *dest, i_file *f)
{
  ASSERT (dest);
  i_file_assert (f);

  const global_config *c = get_global_config ();
  dest->header_size = c->header_size;
  dest->f = f;
  dest->page_size = c->page_size;

  err_t ret = fpgr_set_len (dest);
  if (ret)
    {
      i_log_warn ("Failed to get file size when "
                  "initializing file pager\n");
      return ret;
    }

  return SUCCESS;
}

TEST (fpgr_create)
{
  set_global_config (2048, 10);
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file *fp = i_mkstemp (tmpl);
  test_fail_if_null (fp);
  test_fail_if (i_unlink (tmpl));
  const global_config *c = get_global_config ();
  file_pager pager;

  // edge: file shorter than header
  test_fail_if (i_truncate (fp, c->header_size - 1));
  test_assert_int_equal (fpgr_create (&pager, fp), ERR_INVALID_STATE);

  // edge: file size = header + half a page
  test_fail_if (i_truncate (fp, c->header_size + c->page_size / 2));
  test_assert_int_equal (fpgr_create (&pager, fp), ERR_INVALID_STATE);

  // happy: file exactly header size, zero pages
  test_fail_if (i_truncate (fp, c->header_size));
  test_assert_int_equal (fpgr_create (&pager, fp), SUCCESS);
  test_assert_int_equal (pager.header_size, c->header_size);
  test_assert_int_equal (pager.page_size, c->page_size);
  test_assert_int_equal ((int)pager.npages, 0);

  // happy: file exactly header size, more pages
  test_fail_if (i_truncate (fp, c->header_size + 3 * c->page_size));
  test_assert_int_equal (fpgr_create (&pager, fp), SUCCESS);
  test_assert_int_equal (pager.header_size, c->header_size);
  test_assert_int_equal (pager.page_size, c->page_size);
  test_assert_int_equal ((int)pager.npages, 3);

  test_fail_if (i_close (fp));
}

err_t
fpgr_new (file_pager *p, u64 *dest)
{
  file_pager_assert (p);
  ASSERT (dest);

  u64 newsize = p->header_size + (p->page_size) * (p->npages + 1);
  err_t ret = i_truncate (p->f, newsize);
  if (ret)
    {
      return ret;
    }

  *dest = p->npages++;

  return SUCCESS;
}

TEST (fpgr_new)
{
  set_global_config (2048, 10);
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file *fp = i_mkstemp (tmpl);
  test_fail_if_null (fp);
  test_fail_if (i_unlink (tmpl));
  const global_config *c = get_global_config ();

  test_fail_if (i_truncate (fp, c->header_size));

  file_pager p;
  test_fail_if (fpgr_create (&p, fp));

  u64 pgno;

  test_fail_if (fpgr_new (&p, &pgno));
  test_assert_int_equal ((int)pgno, 0);
  test_assert_int_equal ((int)p.npages, 1);
  test_assert_int_equal ((int)i_file_size (fp),
                         (int)(p.header_size + p.page_size * p.npages));

  test_fail_if (fpgr_new (&p, &pgno));
  test_assert_int_equal ((int)pgno, 1);
  test_assert_int_equal ((int)p.npages, 2);
  test_assert_int_equal ((int)i_file_size (fp),
                         (int)(p.header_size + p.page_size * p.npages));

  test_fail_if (fpgr_new (&p, &pgno));
  test_assert_int_equal ((int)pgno, 2);
  test_assert_int_equal ((int)p.npages, 3);
  test_assert_int_equal ((int)i_file_size (fp),
                         (int)(p.header_size + p.page_size * p.npages));

  test_fail_if (i_close (fp));
}

err_t
fpgr_get_expect (file_pager *p, u8 *dest, u64 pgno)
{
  file_pager_assert (p);
  ASSERT (dest);
  ASSERT (pgno < p->npages);

  // Read all from file
  i64 nread = i_read_all (
      p->f, dest,
      p->page_size,
      pgno * p->page_size);

  if (nread == 0)
    {
      i_log_error ("Early EOF encountered on fpgr_get\n");
      return ERR_INVALID_STATE;
    }

  if (nread < 0)
    {
      i_log_warn ("Invalid read encountered on fpgr_get\n");
      return ERR_IO;
    }

  return SUCCESS;
}

TEST (fpgr_get_expect)
{
  set_global_config (2048, 10);
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file *fp = i_mkstemp (tmpl);
  test_fail_if_null (fp);
  test_fail_if (i_unlink (tmpl));
  const global_config *c = get_global_config ();

  test_fail_if (i_truncate (fp, c->header_size));

  file_pager p;
  test_fail_if (fpgr_create (&p, fp));

  // happy path: new page, write, then read back
  u64 pgno;
  test_fail_if (fpgr_new (&p, &pgno));

  u32 ps = p.page_size;
  u8 *write_buf = i_malloc (ps);
  for (u32 i = 0; i < ps; i++)
    {
      write_buf[i] = (u8)i;
    }
  test_fail_if (fpgr_commit (&p, write_buf, pgno));

  u8 *read_buf = i_malloc (ps);
  i_memset (read_buf, 0xFF, ps);
  test_fail_if (fpgr_get_expect (&p, read_buf, pgno));

  for (u32 i = 0; i < ps; i++)
    {
      test_assert_int_equal (read_buf[i], (u8)i);
    }

  i_free (write_buf);
  i_free (read_buf);
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
  set_global_config (2048, 10);
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file *fp = i_mkstemp (tmpl);
  test_fail_if_null (fp);
  test_fail_if (i_unlink (tmpl));
  const global_config *c = get_global_config ();

  test_fail_if (i_truncate (fp, c->header_size));

  file_pager p;
  test_fail_if (fpgr_create (&p, fp));

  // allocate two pages
  u64 a, b;
  test_fail_if (fpgr_new (&p, &a));
  test_fail_if (fpgr_new (&p, &b));

  // delete valid and out‑of‑range page numbers
  test_fail_if (fpgr_delete (&p, a));
  test_fail_if (fpgr_delete (&p, b + 5));

  test_fail_if (i_close (fp));
}

err_t
fpgr_commit (file_pager *p, const u8 *src, u64 pgno)
{
  file_pager_assert (p);
  ASSERT (src);
  ASSERT (pgno < p->npages);

  err_t ret = i_write_all (
      p->f, src,
      p->page_size,
      pgno * p->page_size);

  // TODO - Figure out when to call invalid_state over err_io
  if (ret)
    {
      i_log_warn ("Failed to write page: %" PRIu64 "\n", pgno);
      return ERR_INVALID_STATE;
    }

  return SUCCESS;
}

TEST (fpgr_commit)
{
  set_global_config (2048, 10);
  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file *fp = i_mkstemp (tmpl);
  test_fail_if_null (fp);
  test_fail_if (i_unlink (tmpl));
  const global_config *c = get_global_config ();

  test_assert_int_equal (i_truncate (fp, c->header_size), 0);

  file_pager p;
  test_fail_if (fpgr_create (&p, fp));

  // allocate one page
  u64 pgno;
  test_fail_if (fpgr_new (&p, &pgno));
  test_assert_int_equal ((int)i_file_size (fp),
                         (int)(p.header_size + p.page_size * p.npages));

  u8 *buf = i_malloc (p.page_size);
  i_memset (buf, 0xAB, p.page_size);

  // happy: writing to new page
  test_fail_if (fpgr_commit (&p, buf, pgno));

  test_fail_if (i_close (fp));
  i_free (buf);
}

///////////////////////////// MEMORY PAGE

DEFINE_DBG_ASSERT_I (memory_page, memory_page, p)
{
  ASSERT (p);
}

void
mp_create (memory_page *dest, u8 *data, u64 pgno)
{
  const global_config *c = get_global_config ();
  dest->data = data;
  i_memset (dest->data, 0, c->page_size);
  dest->data = data;
  dest->pgno = pgno;
}

TEST (mp_create)
{
  set_global_config (2048, 10);
  memory_page m;
  const global_config *c = get_global_config ();
  u8 *data = i_malloc (c->page_size);
  test_fail_if_null (data);

  mp_create (&m, data, 42);

  // fields initialized correctly
  test_assert_int_equal ((int)m.pgno, 42);

  // page memory zeroed out (check first and last byte)
  test_assert_int_equal (data[0], 0);
  test_assert_int_equal (data[c->page_size - 1], 0);

  i_free (data);
}

///////////////////////////// MEMORY PAGER

DEFINE_DBG_ASSERT_I (memory_pager, memory_pager, p)
{
  ASSERT (p);
  ASSERT (p->pages);
  ASSERT (p->cap > 0);
  ASSERT (p->tail < p->cap);
  ASSERT (p->head < p->cap);
}

memory_pager
mpgr_create (memory_page *pages, u32 len)
{
  ASSERT (pages && len > 0);
  memory_pager p;
  p.pages = pages;
  p.cap = len;
  p.head = 0;
  p.tail = 0;
  p.isfull = false;
  i_memset (pages, 0, len * sizeof *pages);
  return p;
}

static inline u32
mpgr_len (const memory_pager *p)
{
  memory_pager_assert (p);
  if (p->isfull)
    {
      return p->cap;
    }
  else
    {
      if (p->head >= p->tail)
        {
          return p->head - p->tail;
        }
      else
        {
          return p->cap - (p->tail - p->head);
        }
    }
}

u8 *
mpgr_get_page (const memory_pager *p, u64 pgno)
{
  memory_pager_assert (p);
  u32 len = mpgr_len (p);

  // TODO: OPTIMIZATION - This can be a hash map
  for (u32 i = 0; i < len; i++)
    {
      u32 idx = (p->tail + i) % p->cap;
      if (p->pages[idx].pgno == pgno)
        {
          return p->pages[idx].data;
        }
    }
  return NULL;
}

TEST (mpgr_get_page)
{
  // TODO
}

u64
mpgr_get_evictable (const memory_pager *p)
{
  memory_pager_assert (p);
  ASSERT (p->isfull);
  return p->pages[p->tail].pgno;
}

void
mpgr_evict (memory_pager *p)
{
  memory_pager_assert (p);
  ASSERT (p->isfull);
  p->tail = (p->tail + 1) % p->cap;
  p->isfull = false;
}

TEST (mpgr_evict)
{
  // TODO
}

///////////////////////////// PAGE TYPES

DEFINE_DBG_ASSERT_I (data_list, data_list, d)
{
  ASSERT (d);
  ASSERT (d->next);
  ASSERT (d->len_num);
  ASSERT (d->len_denom);
  ASSERT (d->data);
}

err_t
page_read_expect (page *dest, page_type expected, u8 *raw)
{
  ASSERT (dest);
  ASSERT (raw);

  dest->raw = raw;

  // All pages have a header in the first 8 bytes
  u32 idx = 0;

#define advance(vname, type)     \
  do                             \
    {                            \
      vname = (type *)&raw[idx]; \
      idx += sizeof *vname;      \
    }                            \
  while (0)

  advance (dest->header, u8);

  if (*dest->header != expected)
    {
      i_log_error ("Expected page type: %d, got page type: %d\n",
                   expected, *dest->header);
      return ERR_INVALID_STATE;
    }

  switch (expected)
    {
    case PG_DATA_LIST:
      {
        advance (dest->dl.next, i64);
        advance (dest->dl.len_num, u16);
        advance (dest->dl.len_denom, u16);
        advance (dest->dl.data, u8);
        break;
      }
    case PG_INNER_NODE:
      {
        advance (dest->in.nkeys, u16);
        advance (dest->in.leafs, u64);
        advance (dest->in.keys, u64); // TODO - this should be at the end
        break;
      }
    case PG_HASH_PAGE:
      {
        advance (dest->hp.hashes, i64);
        break;
      }
    case PG_HASH_LEAF:
      {
        advance (dest->hl.next, u64);
        advance (dest->hl.nvalues, u16);
        advance (dest->hl.offsets, u16);
        break;
      }
    }

#undef advance

  return SUCCESS;
}

void
page_init (page *dest, page_type type, u8 *raw)
{
  ASSERT (dest);
  ASSERT (type > 0);
  ASSERT (raw);

  const global_config *c = get_global_config ();
  i_memset (raw, 0, c->page_size);

  // Set header and raw pointers
  u32 idx = 0;
  dest->header = &raw[idx];
  idx += sizeof *dest->header;
  dest->raw = raw;

  // Set header value
  *dest->header = (u8)type;

  // Read in data - TODO - specialized initializations
  ASCOPE (err_t ret =)
  page_read_expect (dest, type, raw);
  ASSERT (ret == SUCCESS);
}

///////////////////////////// PAGER

DEFINE_DBG_ASSERT_I (pager, pager, p)
{
  ASSERT (p);
  memory_pager_assert (&p->mpager);
  file_pager_assert (&p->fpager);
}
