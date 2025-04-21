#include "database.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "paging.h"
#include "sds.h"
#include "types.h"

//////////////// Global Config
static global_config c;

DEFINE_DBG_ASSERT_I (global_config, global_config, g)
{
  ASSERT (g);
  ASSERT (g->page_size > 0);
  ASSERT (g->mpgr_len > 0);
  ASSERT (g->header_size == sizeof (u32) * 2);
}

void
set_global_config (u32 page_size, u32 mpgr_len)
{
  c.header_size = sizeof (u32) * 2;
  c.page_size = page_size;
  c.mpgr_len = mpgr_len;
}

const global_config *
get_global_config (void)
{
  global_config_assert (&c);
  return &c;
}

//////////////// Global Config
static global_database db;

DEFINE_DBG_ASSERT_I (global_database, global_database, g)
{
  ASSERT (g);
  pager_assert (&g->pager);
  ASSERT (g->resources._data);
  i_file_assert (&g->resources.fp);
}

global_database *
get_global_database (void)
{
  global_database_assert (&db);
  return &db;
}

err_t
db_create (const string fname, u32 page_size, u32 mpgr_len)
{
  string_assert (&fname);
  set_global_config (page_size, mpgr_len);
  db.alloc = stdalloc_create (1e10);

  if (i_exists_rw (fname))
    {
      return ERR_ALREADY_EXISTS;
    }

  i_file fp;
  err_t_wrap (i_open (&fp, fname, 1, 1));

  err_t ret = SUCCESS;

  // write header
  u32 header[2] = { c.page_size, c.mpgr_len };
  ret = i_write_all (&fp, header, sizeof (header), 0);
  if (ret)
    {
      goto cleanup;
    }

  // initialize file pager
  file_pager fpgr;
  ret = fpgr_create (&fpgr, fp, c.page_size, c.header_size);
  if (ret)
    {
      goto cleanup;
    }

  // allocate and commit hash page
  u64 pgno;
  ret = fpgr_new (&fpgr, &pgno);
  if (ret)
    {
      goto cleanup;
    }
  ASSERT (pgno == 0);

  alloc_ret a = db.alloc.alloc.malloc (&db.alloc.alloc, c.page_size);
  if (a.ret)
    {
      ret = a.ret;
      goto cleanup;
    }
  u8 *buf = a.data;

  page p;
  page_init (&p, PG_HASH_PAGE, buf, c.page_size, pgno);
  ret = fpgr_commit (&fpgr, buf, pgno);
  if (ret)
    {
      goto cleanup;
    }

  ret = SUCCESS;

cleanup:

  i_close (&fp);
  return ret;
}

err_t
db_open (const string fname)
{
  cstring_assert (&fname);
  i_memset (&db, 0, sizeof (db));
  db.alloc = stdalloc_create (1e10);

  // verify file exists
  if (!i_exists_rw (fname))
    {
      return ERR_ALREADY_EXISTS;
    }

  // open file
  err_t_wrap (i_open (&db.resources.fp, fname, 1, 1));

  // read header
  u32 header[2] = { 0 };
  err_t bytes = i_read_all (&db.resources.fp, header, sizeof (header), 0);
  if (bytes != sizeof (header))
    {
      return ERR_IO;
    }
  set_global_config (header[0], header[1]);

  // memory pager
  u64 total = c.mpgr_len * (c.page_size + sizeof (memory_pager));

  alloc_ret a = db.alloc.alloc.malloc (&db.alloc.alloc, total);
  if (a.ret)
    {
      return a.ret;
    }
  u8 *block = a.data;

  u8 *ptr = block;
  memory_page *pages = (memory_page *)ptr;
  ptr += c.mpgr_len * sizeof (memory_pager);
  for (u32 i = 0; i < c.mpgr_len; ++i)
    {
      pages[i].data = ptr + i * c.page_size;
    }
  db.resources._data = block;
  db.mpgr = mpgr_create (pages, c.mpgr_len);

  // init pager
  err_t ret = fpgr_create (&db.fpgr, db.resources.fp, c.page_size, c.header_size);
  if (ret)
    {
      return ret;
    }
  db.pager = pgr_create (db.mpgr, db.fpgr, c.page_size);

  return SUCCESS;
}

void
db_close (void)
{
  db.alloc.alloc.free (&db.alloc.alloc, db.resources._data);
  i_close (&db.resources.fp);
}
