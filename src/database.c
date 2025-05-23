#include "database.h"

#include "ast/query/query_provider.h"
#include "dev/assert.h"              // DEFINE_DBG_ASSERT_I
#include "intf/io.h"                 // i_touch
#include "variables/vfile_hashmap.h" // vfile_hashmap

DEFINE_DBG_ASSERT_I (database, database, d)
{
  ASSERT (d);
  ASSERT (d->pager);
  ASSERT (d->qspce);
}

err_t
db_create (const string fname, error *e)
{
  if (i_exists_rw (fname))
    {
      return error_causef (
          e, ERR_ALREADY_EXISTS,
          "Trying to create a database: %.*s "
          "but that file already exists",
          fname.len, fname.data);
    }

  // Create the file
  err_t_wrap (i_touch (fname, e), e);

  // Wrap it in a pager
  pager *p = pgr_create (fname, e);
  if (p == NULL)
    {
      goto failed_rm_file;
    }

  // Create the first hash page
  vfile_hashmap hm = vfhm_create (p);
  if (vfhm_create_hashmap (&hm, e))
    {
      goto failed_rm_file;
    }

  pgr_close (p);

  return SUCCESS;

failed_rm_file:
  err_t_log_swallow (i_remove_quiet (fname, &tempe), tempe);
  return err_t_from (e);
}

err_t
db_open (database *dest, const string fname, error *e)
{
  ASSERT (dest);
  ASSERT (fname.data);
  ASSERT (fname.len > 0);

  pager *p = pgr_create (fname, e);
  if (p == NULL)
    {
      return err_t_from (e);
    }

  query_provider *qspce = query_provider_create (e);
  if (qspce == NULL)
    {
      pgr_close (p);
      return err_t_from (e);
    }

  dest->qspce = qspce;
  dest->pager = p;

  return SUCCESS;
}

void
db_close (database *d)
{
  database_assert (d);
  pgr_close (d->pager);
  query_provider_free (d->qspce);
}
