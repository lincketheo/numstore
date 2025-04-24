#include "database.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "paging.h"
#include "sds.h"
#include "types.h"
#include "vhash_map.h"

//////////////// Global Config
DEFINE_DBG_ASSERT_I (database, database, g)
{
  ASSERT (g);
  pager_assert (&g->pager);
  ASSERT (g->resources._data);
  i_file_assert (&g->resources.fp);
}

err_t
db_create (const string fname, u32 page_size, u32 mpgr_len)
{
  string_assert (&fname);

  database db;

  // Check if file already exists
  if (i_exists_rw (fname))
    {
      return ERR_ALREADY_EXISTS;
    }

  // Open the file or return error
  err_t_wrap (i_open (&db.resources.fp, fname, 1, 1));

  // The return value
  err_t ret;

  // Write the header
  u32 header[2] = { page_size, mpgr_len };
  if ((ret = i_write_all (&db.resources.fp, header, sizeof (header), 0)))
    {
      goto cleanup;
    }

  // Set configured parameters
  db.page_size = page_size;
  db.mpgr_len = mpgr_len;
  db.header_size = 2 * sizeof (u32);

  // Create allocator for internal db resources
  db.alloc = lalloc_create (db.page_size);

  // Open a file pager
  file_pager fpgr;
  if ((ret = fpgr_create (
           &fpgr,
           db.resources.fp,
           db.page_size,
           db.header_size)))
    {
      goto cleanup;
    }

  // Allocate and commit the first hash page
  {
    // Allocate room in the file
    u64 pgno;
    ret = fpgr_new (&fpgr, &pgno);
    if (ret)
      {
        goto cleanup;
      }
    ASSERT (pgno == 0);

    // Create the hash page
    u8 *buf = lmalloc (&db.alloc, db.page_size);
    if (!buf)
      {
        goto cleanup;
      }

    page p;
    page_init (&p, PG_HASH_PAGE, buf, db.page_size, pgno);
    if ((ret = fpgr_commit (&fpgr, buf, pgno)))
      {
        goto cleanup;
      }
  }

cleanup:

  i_close (&db.resources.fp);
  return ret;
}

err_t
db_open (database *db, const string fname)
{
  cstring_assert (&fname);

  err_t ret = SUCCESS;

  // verify file exists
  if (!i_exists_rw (fname))
    {
      return ERR_ALREADY_EXISTS;
    }

  // Open then read header of file
  {
    // open file
    err_t_wrap (i_open (&db->resources.fp, fname, 1, 1));

    // Read it
    u32 header[2] = { 0 };
    i64 bytes = i_read_all (
        &db->resources.fp,
        header,
        sizeof (header), 0);

    // Check if read successful
    if (bytes != sizeof (header))
      {
        return ERR_IO;
      }

    // Set config
    db->page_size = header[0];
    db->mpgr_len = header[1];
    if (db->page_size == 0 || db->mpgr_len == 0)
      {
        return ERR_INVALID_STATE;
      }
  }

  // Create allocator for internal resources
  u64 total = db->mpgr_len * (db->page_size + sizeof (memory_pager));
  db->alloc = lalloc_create (total);

  // Initialize a memory pager
  memory_pager mpgr;
  {
    // Allocate memory for memory pages
    if (!(db->resources._data = lmalloc (&db->alloc, total)))
      {
        return ERR_NOMEM;
      }

    // Initialize the memory_pages
    u8 *ptr = db->resources._data;
    memory_page *pages = (memory_page *)ptr;
    ptr += db->mpgr_len * sizeof (memory_pager);
    for (u32 i = 0; i < db->mpgr_len; ++i)
      {
        pages[i].data = ptr + i * db->page_size;
      }

    // Create
    mpgr = mpgr_create (pages, db->mpgr_len);
  }

  // Initialize a file pager
  file_pager fpgr;
  if ((ret = fpgr_create (
           &fpgr,
           db->resources.fp,
           db->page_size,
           db->header_size)))
    {
      return ret;
    }

  // Initialize the full pager
  db->pager = pgr_create (mpgr, fpgr, db->page_size);

  // Initialize hash map -
  // TODO - don't use the db allocator here
  if ((ret = vhash_map_create (
           &db->variables,
           db->page_size,
           &db->alloc)))
    {
      return ret;
    }

  return SUCCESS;
}

void
db_close (database *db)
{
  database_assert (db);
  lfree (&db->alloc, db->resources._data);
  i_close (&db->resources.fp);
}

lalloc *
db_request_alloc (database *db, u32 limit)
{
  database_assert (db);
  ASSERT (limit > 0);

  u32 idx;

  // First scan allocators to see if any of them
  // are available
  for (u32 i = 0; i < db->allocators.len; ++i)
    {
      if (!db->allocators.is_present[i])
        {
          idx = i;
          goto theend;
        }
    }

  // Next append allocator to the end of the list
  // Check if len exceeds capacity
  if (db->allocators.len == db->allocators.cap)
    {
      lalloc *allocs = lrealloc (
          &db->alloc,
          db->allocators.allocs,
          2 * db->allocators.cap);
      if (!allocs)
        {
          return NULL;
        }
      db->allocators.allocs = allocs;
      db->allocators.cap *= 2;
    }

  idx = db->allocators.len++;

theend:
  db->allocators.allocs[idx] = lalloc_create (limit);
  db->allocators.is_present[idx] = true;
  return &db->allocators.allocs[idx];
}

void
db_release_alloc (database *db, lalloc *alloc)
{
  lalloc_assert (alloc);

  for (u32 i = 0; i < db->allocators.len; ++i)
    {
      if (&db->allocators.allocs[i] == alloc)
        {
          ASSERT (db->allocators.is_present[i]);
          db->allocators.is_present[i] = false;
          return;
        }
    }
  ASSERT (0);
}
