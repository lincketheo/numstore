#include "database.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "intf/types.h"
#include "vhash_map.h"

//////////////// Global Config
DEFINE_DBG_ASSERT_I (database, database, g)
{
  ASSERT (g);
  // TODO
}

static inline err_t
db_create_file (database *db, dbcargs args)
{
  // The return value
  err_t ret;

  // Check if file already exists
  if (i_exists_rw (args.fname))
    {
      return ERR_ALREADY_EXISTS;
    }

  // Open the file or return error
  if ((ret = i_open (&db->private.fp, args.fname, 1, 1)))
    {
      return ret;
    }

  // Write the header
  u32 header[2] = { args.page_size, args.mpgr_len };
  db->private.header_size = sizeof (header);
  if ((ret = i_write_all (&db->private.fp, header, sizeof (header), 0)))
    {
      return ret;
    }

  // Set configured parameters
  db->page_size = args.page_size;
  db->private.mpgr_len = args.mpgr_len;
  db->private.header_size = 2 * sizeof (u32);

  if ((ret = i_close (&db->private.fp)))
    {
      return ret;
    }

  return SUCCESS;
}

static inline err_t
db_open_file (database *db, dboargs args)
{
  err_t ret = SUCCESS;

  // verify file exists
  if (!i_exists_rw (args.fname))
    {
      return ERR_DOESNT_EXIST;
    }

  // Open the file
  if ((ret = i_open (&db->private.fp, args.fname, 1, 1)))
    {
      return ret;
    }

  // Read it
  u32 header[2] = { 0 };
  db->private.header_size = sizeof (header);
  i64 bytes = i_read_all (&db->private.fp, header, sizeof (header), 0);
  if (bytes != sizeof (header))
    {
      goto invalid_state;
    }

  // Set config
  db->page_size = header[0];
  db->private.mpgr_len = header[1];

  // Check validity of file
  if (db->page_size == 0 || db->private.mpgr_len == 0)
    {
      goto invalid_state;
    }

  // TODO - probably more validity checks to be done here

  return ret;

invalid_state:
  i_close (&db->private.fp);
  return ERR_INVALID_STATE;
}

err_t
db_create (database *db, dbcargs args)
{
  err_t ret;
  if ((ret = db_create_file (db, args)))
    {
      return ret;
    }

  dboargs _args = (dboargs){
    .fname = args.fname,
  };

  return db_open (db, _args);
}

err_t
db_open (database *db, dboargs args)
{
  err_t ret;

  if ((ret = db_open_file (db, args)))
    {
      return ret;
    }

  if ((ret = lalloc_create (&db->private.alloc, 100, 100)))
    {
      // TODO - cleanup
      return ret;
    }

  // Initialize a memory pager
  memory_pager mpgr;
  {
    memory_page *pages = mp_list_create (
        db->page_size,
        db->private.mpgr_len,
        &db->private.alloc);
    if (pages == NULL)
      {
        // TODO - cleanup
        return ERR_NOMEM;
      }
    mpgr = mpgr_create (pages, db->private.mpgr_len);
  }

  // Initialize a file pager
  file_pager fpgr;
  if ((ret = fpgr_create (
           &fpgr,
           db->private.fp,
           db->page_size,
           db->private.header_size)))
    {
      // TODO - cleanup
      return ret;
    }

  // Merge into 1 pager
  db->pager = pgr_create (mpgr, fpgr, db->page_size);

  // Initialize hash map
  if ((ret = vhash_map_create (
           &db->variables,
           db->page_size,
           &db->private.alloc)))
    {
      // TODO - cleanup
      return ret;
    }

  return SUCCESS;
}

void
db_close (database *db)
{
  database_assert (db);
  lalloc_free (&db->private.alloc);
  i_close (&db->private.fp);
}

err_t
db_request_alloc (lalloc **dest, database *db, u64 climit, u32 dlimit)
{
  ASSERT (dest);
  database_assert (db);

  u32 idx;
  err_t ret;

  // First scan allocators to see if any of them
  // are available
  for (u32 i = 0; i < db->private.allocators.len; ++i)
    {
      if (!db->private.allocators.is_present[i])
        {
          idx = i;
          goto theend;
        }
    }

  // Next append allocator to the end of the list
  // Check if len exceeds capacity
  if (db->private.allocators.len == db->private.allocators.cap)
    {
      lalloc *allocs = lrealloc_dyn (
          &db->private.alloc,
          db->private.allocators.allocs,
          2 * db->private.allocators.cap);
      if (!allocs)
        {
          return ERR_NOMEM;
        }
      db->private.allocators.allocs = allocs;
      db->private.allocators.cap *= 2;
    }

  idx = db->private.allocators.len++;

theend:

  if ((ret = lalloc_create (
           &db->private.allocators.allocs[idx],
           climit, dlimit)))
    {
      return ret;
    }
  db->private.allocators.is_present[idx] = true;
  *dest = &db->private.allocators.allocs[idx];
  return SUCCESS;
}

void
db_release_alloc (database *db, lalloc *alloc)
{
  database_assert (db);

  for (u32 i = 0; i < db->private.allocators.len; ++i)
    {
      if (&db->private.allocators.allocs[i] == alloc)
        {
          ASSERT (db->private.allocators.is_present[i]);
          lalloc_free (alloc);
          db->private.allocators.is_present[i] = false;
          return;
        }
    }
  ASSERT (0);
}

err_t
db_request_cbuffer (cbuffer **dest, database *db, u32 cap)
{
  ASSERT (dest);
  database_assert (db);

  u32 idx;

  // First scan to see if any of them are available
  for (u32 i = 0; i < db->private.allocators.len; ++i)
    {
      if (db->private.cbuffers.cbuffers[i].data == NULL)
        {
          idx = i;
          goto theend;
        }
    }

  // Next append allocator to the end of the list
  // Check if len exceeds capacity
  if (db->private.cbuffers.len == db->private.cbuffers.cap)
    {
      cbuffer *cbuffers = lrealloc_dyn (
          &db->private.alloc,
          db->private.cbuffers.cbuffers,
          2 * db->private.cbuffers.cap);
      if (!cbuffers)
        {
          return ERR_NOMEM;
        }
      db->private.cbuffers.cbuffers = cbuffers;
      db->private.cbuffers.cap *= 2;
    }

  idx = db->private.cbuffers.len++;

theend:
  {
    u8 *data = lmalloc_dyn (&db->private.alloc, cap);
    if (!data)
      {
        return ERR_NOMEM;
      }

    db->private.cbuffers.cbuffers[idx] = cbuffer_create (data, cap);
    *dest = &db->private.cbuffers.cbuffers[idx];
    return SUCCESS;
  }
}

void
db_release_cbuffer (database *db, cbuffer *c)
{
  database_assert (db);

  for (u32 i = 0; i < db->private.cbuffers.len; ++i)
    {
      cbuffer *_c = &db->private.cbuffers.cbuffers[i];
      if (_c == c)
        {
          lfree_dyn (&db->private.alloc, _c->data);
          _c->data = NULL;
          return;
        }
    }
  ASSERT (0);
}
