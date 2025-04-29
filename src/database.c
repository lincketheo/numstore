#include "database.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "intf/types.h"
#include "paging/pager.h"
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
  if ((ret = i_open (&db->fp, args.fname, 1, 1)))
    {
      return ret;
    }

  // Write the header
  u32 header[2] = { args.page_size, args.mpgr_len };
  db->header_size = sizeof (header);
  if ((ret = i_pwrite_all (&db->fp, header, sizeof (header), 0)))
    {
      return ret;
    }

  // Set configured parameters
  db->page_size = args.page_size;
  db->mpgr_len = args.mpgr_len;
  db->header_size = 2 * sizeof (u32);

  if ((ret = i_close (&db->fp)))
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
  if ((ret = i_open (&db->fp, args.fname, 1, 1)))
    {
      return ret;
    }

  // Read it
  u32 header[2] = { 0 };
  db->header_size = sizeof (header);
  i64 bytes = i_pread_all (&db->fp, header, sizeof (header), 0);
  if (bytes != sizeof (header))
    {
      goto invalid_state;
    }

  // Set config
  db->page_size = header[0];
  db->mpgr_len = header[1];

  // Check validity of file
  if (db->page_size == 0 || db->mpgr_len == 0)
    {
      goto invalid_state;
    }

  // TODO - probably more validity checks to be done here

  return ret;

invalid_state:
  i_close (&db->fp);
  return ERR_INVALID_STATE;
}

bool
db_exists (const string fname)
{
  if (i_exists_rw (fname))
    {
      return true;
    }
  return false;
}

err_t
db_create_and_open (database *db, dbcargs args)
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

  lalloc_create (&db->alloc, 1000000);

  // Create the pager
  pgr_params params = {
    .page_size = db->page_size,
    .f = db->fp,
    .header_size = db->header_size,
    .memory_pager_len = db->mpgr_len,
    .memory_pager_allocator = &db->alloc,
  };

  if ((ret = pgr_create (&db->pager, params)))
    {
      return ret;
    }

  // Create the variable hash map
  vhm_params vparams = {
    .len = db->page_size,
    .map_allocator = &db->alloc,
    .node_allocator = &db->alloc,
    .type_allocator = &db->alloc,
  };
  if ((ret = vhash_map_create (&db->variables, vparams)))
    {
      return ret;
    }

  return SUCCESS;
}

void
db_close (database *db)
{
  database_assert (db);
  // lalloc_release (&db->alloc);
  i_close (&db->fp);
}
