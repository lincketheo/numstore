#include "database.h"

#include "ast/query/query_provider.h" // qspce
#include "dev/assert.h"               // DEFINE_DBG_ASSERT_I
#include "intf/io.h"                  // i_touch
#include "paging/pager.h"             // pager

struct database_s
{
  pager *pager;
  query_provider *qspce;
};

DEFINE_DBG_ASSERT_I (database, database, d)
{
  ASSERT (d);
  ASSERT (d->pager);
  ASSERT (d->qspce);
}

static const char *TAG = "Database";

database *
db_open (const string fname, error *e)
{

  database *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "%s Failed to allocate database", TAG);
      return NULL;
    }

  // Build pager
  pager *p = pgr_open (fname, e);
  if (p == NULL)
    {
      i_free (ret);
      return NULL;
    }

  // Build query space provider
  query_provider *qspce = query_provider_create (e);
  if (qspce == NULL)
    {
      i_free (ret);
      err_t_log_swallow (pgr_close (p, &_e), _e);
      return NULL;
    }

  ret->pager = p;
  ret->qspce = qspce;

  return ret;
}

void
db_close (database *d)
{
  database_assert (d);
  err_t_log_swallow (pgr_close (d->pager, &_e), _e);
  query_provider_free (d->qspce);
}
