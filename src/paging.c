#include "paging.h"
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
  i_file_assert (&p->f);
}

static inline err_t
fpgr_set_len (file_pager *p)
{
  file_pager_assert (p);
  i64 size = i_file_size (&p->f); // TODO - can file size > i64?
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
fpgr_create (
    file_pager *dest,
    i_file f,
    u32 page_size,
    u32 header_size)
{
  ASSERT (dest);
  i_file_assert (&f);

  dest->header_size = header_size;
  dest->f = f;
  dest->page_size = page_size;

  err_t ret = fpgr_set_len (dest);
  if (ret)
    {
      return ret;
    }

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
  test_assert_int_equal (fpgr_create (&pager, fp, 2048, 10), ERR_INVALID_STATE);

  // edge: file size = header + half a page
  test_fail_if (i_truncate (&fp, 10 + 2048 / 2));
  test_assert_int_equal (fpgr_create (&pager, fp, 2048, 10), ERR_INVALID_STATE);

  // happy: file exactly header size, zero pages
  test_fail_if (i_truncate (&fp, 10));
  test_assert_int_equal (fpgr_create (&pager, fp, 2048, 10), SUCCESS);
  test_assert_int_equal (pager.header_size, 10);
  test_assert_int_equal (pager.page_size, 2048);
  test_assert_int_equal ((int)pager.npages, 0);

  // happy: file exactly header size, more pages
  test_fail_if (i_truncate (&fp, 10 + 3 * 2048));
  test_assert_int_equal (fpgr_create (&pager, fp, 2048, 10), SUCCESS);
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
  test_fail_if (fpgr_create (&p, fp, 2048, 10));

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
  i64 nread = i_read_all (
      &p->f, dest,
      p->page_size,
      p->header_size + pgno * p->page_size);

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
  stdalloc salloc = stdalloc_create (0);
  lalloc *alloc = &salloc.alloc;

  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  test_fail_if (i_mkstemp (&fp, tmpl));
  test_fail_if (i_unlink (tmpl));

  test_fail_if (i_truncate (&fp, 10));

  file_pager p;
  test_fail_if (fpgr_create (&p, fp, 2048, 10));

  // happy path: new page, write, then read back
  u64 pgno;
  test_fail_if (fpgr_new (&p, &pgno));

  u32 ps = p.page_size;
  alloc_ret a = alloc->malloc (alloc, ps);
  test_fail_if (a.ret);
  u8 *write_buf = a.data;
  for (u32 i = 0; i < ps; i++)
    {
      write_buf[i] = (u8)i;
    }
  test_fail_if (fpgr_commit (&p, write_buf, pgno));

  a = alloc->malloc (alloc, ps);
  test_fail_if (a.ret);
  u8 *read_buf = a.data;
  i_memset (read_buf, 0xFF, ps);
  test_fail_if (fpgr_get_expect (&p, read_buf, pgno));

  for (u32 i = 0; i < ps; i++)
    {
      test_assert_int_equal (read_buf[i], (u8)i);
    }

  alloc->free (alloc, write_buf);
  alloc->free (alloc, read_buf);
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
  test_fail_if (fpgr_create (&p, fp, 2048, 10));

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

  err_t ret = i_write_all (
      &p->f, src,
      p->page_size,
      p->header_size + pgno * p->page_size);

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
  stdalloc salloc = stdalloc_create (4000);
  lalloc *alloc = &salloc.alloc;

  char _tmpl[] = "/tmp/fpgr_testXXXXXX";
  string tmpl = unsafe_cstrfrom (_tmpl);
  i_file fp;
  test_fail_if (i_mkstemp (&fp, tmpl));
  test_fail_if (i_unlink (tmpl));

  test_fail_if (i_truncate (&fp, 10));

  file_pager p;
  test_fail_if (fpgr_create (&p, fp, 2048, 10));

  // allocate one page
  u64 pgno;
  test_fail_if (fpgr_new (&p, &pgno));
  test_assert_int_equal ((int)i_file_size (&fp),
                         (int)(p.header_size + p.page_size * p.npages));

  alloc_ret a = alloc->malloc (alloc, p.page_size);
  test_fail_if (a.ret);
  u8 *buf = a.data;

  i_memset (buf, 0xAB, p.page_size);

  // happy: writing to new page
  test_fail_if (fpgr_commit (&p, buf, pgno));

  test_fail_if (i_close (&fp));
  alloc->free (alloc, buf);
}

///////////////////////////// MEMORY PAGE

DEFINE_DBG_ASSERT_I (memory_page, memory_page, p)
{
  ASSERT (p);
  ASSERT (p->data);
}

void
mp_create (memory_page *dest, u8 *data, u32 dlen, u64 pgno)
{
  dest->data = data;
  i_memset (dest->data, 0, dlen);
  dest->data = data;
  dest->pgno = pgno;
}

TEST (mp_create)
{
  stdalloc salloc = stdalloc_create (4000);
  lalloc *alloc = &salloc.alloc;

  memory_page m;

  alloc_ret a = alloc->malloc (alloc, 2048);
  test_fail_if (a.ret);
  u8 *data = a.data;

  test_fail_if_null (data);

  mp_create (&m, data, 2048, 42);

  // fields initialized correctly
  test_assert_int_equal ((int)m.pgno, 42);

  // page memory zeroed out (check first and last byte)
  test_assert_int_equal (data[0], 0);
  test_assert_int_equal (data[2048 - 1], 0);

  alloc->free (alloc, data);
  alloc->release (alloc);
}

///////////////////////////// MEMORY PAGER

DEFINE_DBG_ASSERT_I (memory_pager, memory_pager, p)
{
  ASSERT (p);
  ASSERT (p->pages);
  ASSERT (p->len > 0);
  for (u32 i = 0; i < p->len; ++i)
    {
      memory_page_assert (&p->pages[i]);
    }
}

memory_pager
mpgr_create (memory_page *pages, u32 len)
{
  ASSERT (pages && len > 0);
  memory_pager p;
  p.pages = pages;
  p.len = len;
  p.idx = 0;
  memory_pager_assert (&p);
  return p;
}

u8 *
mpgr_new (memory_pager *p, u64 pgno)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < p->len; ++i)
    {

      memory_page *mp = &p->pages[p->idx];
      p->idx = (p->idx + 1) % p->len;

      if (!mp->is_present)
        {
          mp->pgno = pgno;
          mp->is_present = 1;
          return mp->data;
        }
    }

  return NULL;
}

u8 *
mpgr_get (const memory_pager *p, u64 pgno)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < p->len; ++i)
    {

      memory_page *mp = &p->pages[(i + p->idx) % p->len];

      if (mp->is_present && mp->pgno == pgno)
        {
          return mp->data;
        }
    }

  return NULL;
}

u64
mpgr_get_evictable (const memory_pager *p)
{
  memory_pager_assert (p);
  memory_page *mp = &p->pages[p->idx];
  ASSERT (mp->is_present); // Called on a full buffer
  return mp->pgno;
}

void
mpgr_evict (memory_pager *p, u64 pgno)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < p->len; ++i)
    {

      memory_page *mp = &p->pages[p->idx];

      if (mp->is_present && mp->pgno == pgno)
        {
          mp->is_present = 0;
          return;
        }

      p->idx = (p->idx + 1) % p->len;
    }
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

DEFINE_DBG_ASSERT_H (inner_node, inner_node, d)
{
  ASSERT (d);
  // TODO
}

DEFINE_DBG_ASSERT_H (hash_page, hash_page, d)
{
  ASSERT (d);
  // TODO
}

DEFINE_DBG_ASSERT_H (hash_leaf, hash_leaf, d)
{
  ASSERT (d);
  // TODO
}

DEFINE_DBG_ASSERT_H (page, page, p)
{
  ASSERT (p);
  ASSERT (p->raw);
  ASSERT (p->header);
  switch (*p->header)
    {
    case PG_DATA_LIST:
      data_list_assert (&p->dl);
      break;
    case PG_INNER_NODE:
      inner_node_assert (&p->in);
      break;
    case PG_HASH_PAGE:
      hash_page_assert (&p->hp);
      break;
    case PG_HASH_LEAF:
      hash_leaf_assert (&p->hl);
      break;
    default:
      ASSERT (0);
    }
}

static inline void
page_set (
    page *p,
    page_type type,
    u8 *raw,
    u32 rlen,
    u64 pgno)
{
  // All pages have a header in the first 8 bytes
  u32 idx = 0;
  p->raw = raw;
  p->pgno = pgno;
  p->len = rlen;

#define advance(vname, type)        \
  do                                \
    {                               \
      vname = (type *)&p->raw[idx]; \
      idx += sizeof *vname;         \
    }                               \
  while (0)

  advance (p->header, u8);

  switch (type)
    {
    case PG_DATA_LIST:
      {
        advance (p->dl.next, i64);
        advance (p->dl.len_num, u16);
        advance (p->dl.len_denom, u16);
        advance (p->dl.data, u8);
        break;
      }
    case PG_INNER_NODE:
      {
        advance (p->in.nkeys, u16);
        advance (p->in.leafs, u64);
        advance (p->in.keys, u64); // TODO - this should be at the end
        break;
      }
    case PG_HASH_PAGE:
      {
        advance (p->hp.len, u32);
        advance (p->hp.hashes, u64);
        break;
      }
    case PG_HASH_LEAF:
      {
        advance (p->hl.next, u64);
        advance (p->hl.nvalues, u16);
        advance (p->hl.offsets, u16);
        break;
      }
    }

#undef advance
}

err_t
page_read_expect (
    page *dest,
    page_type expected,
    u8 *raw,
    u32 rlen,
    u64 pgno)
{
  ASSERT (dest);
  ASSERT (raw);

  page_set (dest, expected, raw, rlen, pgno);

  if (*dest->header != expected)
    {
      i_log_error ("Expected page type: %d, got page type: %d\n",
                   expected, *dest->header);
      return ERR_INVALID_STATE;
    }

  return SUCCESS;
}

void
page_init (page *dest, page_type type, u8 *raw, u32 rlen, u64 pgno)
{
  ASSERT (dest);
  ASSERT (type > 0);
  ASSERT (raw);

  i_memset (raw, 0, rlen);

  page_set (dest, type, raw, rlen, pgno);
  *dest->header = (u8)type;

  // Aditional Setup
  switch (type)
    {
    case PG_DATA_LIST:
      {
        break;
      }
    case PG_INNER_NODE:
      {
        break;
      }
    case PG_HASH_PAGE:
      {
        ASSERT (rlen > sizeof (u32));
        *dest->hp.len = (rlen - sizeof (u32)) / sizeof (i64);
        break;
      }
    case PG_HASH_LEAF:
      {
        break;
      }
    }
}

//// HASH LEAF UTILS
err_t
hl_get_tuple (hash_leaf_tuple *dest, const page *p, u16 idx)
{
  ASSERT (dest);
  ASSERT (*p->header == PG_HASH_LEAF);
  page_assert (p);

  u16 nvals = *p->hl.nvalues;
  ASSERT (idx < nvals);

  u8 *base = p->raw;
  u32 off = p->hl.offsets[idx];

#define BOUND_AND_ADVANCE(_len)  \
  do                             \
    {                            \
      if (off + (_len) > p->len) \
        goto overflow;           \
      off += (_len);             \
    }                            \
  while (0)

  // strlen
  BOUND_AND_ADVANCE (sizeof *dest->strlen);
  dest->strlen = (u16 *)(base + off - sizeof *dest->strlen);
  u16 slen = *dest->strlen;

  // str
  BOUND_AND_ADVANCE (slen);
  dest->str = (char *)(base + off - slen);

  // pg0
  BOUND_AND_ADVANCE (sizeof *dest->pg0);
  dest->pg0 = (u64 *)(base + off - sizeof *dest->pg0);

  // tstrlen
  BOUND_AND_ADVANCE (sizeof *dest->tstrlen);
  dest->tstrlen = (u16 *)(base + off - sizeof *dest->tstrlen);
  u16 tlen = *dest->tstrlen;

  // tstr
  BOUND_AND_ADVANCE (tlen);
  dest->tstr = (char *)(base + off - tlen);

#undef BOUND_AND_ADVANCE

  return SUCCESS;

overflow:
  i_log_error ("Malformed page %" PRIu64 " at tuple %u:"
               " offset %u overflows page (%u)\n",
               p->pgno, idx, off, p->len);
  return ERR_INVALID_STATE;
}

TEST (hl_get_tuple)
{
  // Green path
  u8 raw1[64] = { 0 };

  page p1 = {
    .raw = raw1,
    .len = 64,
    .header = raw1,
    .pgno = 42,
    .hl = {
        .nvalues = &(u16){ 1 },
        .offsets = (u16[]){ 8 },
    }
  };
  raw1[0] = PG_HASH_LEAF;

  // Build tuple
  u16 slen = 3;
  i_memcpy (raw1 + 8, &slen, sizeof slen);
  i_memcpy (raw1 + 10, "bar", slen);

  u64 pg0 = 0x12345678;
  i_memcpy (raw1 + 13, &pg0, sizeof pg0);

  u16 tlen = 4;
  i_memcpy (raw1 + 21, &tlen, sizeof tlen);
  i_memcpy (raw1 + 23, "test", tlen);

  hash_leaf_tuple dest;
  err_t r1 = hl_get_tuple (&dest, &p1, 0);

  test_assert_int_equal ((int)r1, (int)SUCCESS);
  test_assert_int_equal ((int)*dest.strlen, (int)slen);
  test_assert_int_equal ((int)*dest.pg0, (int)pg0);
  test_assert_int_equal ((int)*dest.tstrlen, (int)tlen);

  // Red path - overflow
  u8 raw2[16] = { 0 };

  page p2 = {
    .raw = raw2,
    .len = 16,
    .header = raw2,
    .pgno = 99,
    .hl = {
        .nvalues = &(u16){ 1 },
        .offsets = (u16[]){ 12 },
    }
  };
  raw2[0] = PG_HASH_LEAF;

  err_t r2 = hl_get_tuple (&dest, &p2, 0);
  test_assert_int_equal ((int)r2, (int)ERR_INVALID_STATE);
}

///////////////////////////// PAGER

DEFINE_DBG_ASSERT_I (pager, pager, p)
{
  ASSERT (p);
  memory_pager_assert (&p->mpager);
  file_pager_assert (&p->fpager);
}

pager
pgr_create (
    memory_pager mpager,
    file_pager fpager,
    u32 page_size)
{
  memory_pager_assert (&mpager);
  file_pager_assert (&fpager);
  ASSERT (page_size > 0);

  pager dest;

  dest.mpager = mpager;
  dest.fpager = fpager;
  dest.page_size = page_size;

  pager_assert (&dest);

  return dest;
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

  if (page == NULL)
    {
      // Evict a page
      err_t ret = pgr_evict (p);
      if (ret)
        {
          return ret;
        }

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

  return page_read_expect (dest, type, page, p->page_size, pgno);
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
      err_t ret = pgr_evict (p);
      if (ret)
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

  page_init (dest, type, raw, p->page_size, pgno);
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
