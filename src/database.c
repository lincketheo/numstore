#include "database.h"

#include "ast/query/query_provider.h" // qspce
#include "dev/assert.h"               // DEFINE_DBG_ASSERT_I
#include "intf/io.h"                  // i_touch
#include "paging/pager.h"             // pager
#include "virtual_machine.h"          // vm

struct database_s
{
  pager *pager;
  query_provider *qspce;
  vm *vm;
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
  database *ret = NULL;
  pager *p = NULL;
  query_provider *q = NULL;
  vm *v = NULL;

  ret = i_malloc (1, sizeof *ret);
  p = pgr_open (fname, e);
  q = query_provider_create (e);
  v = vm_open (p, e);

  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "%s Failed to allocate database", TAG);
      goto failed;
    }
  if (!(p && q && v))
    {
      goto failed;
    }

  ret->pager = p;
  ret->qspce = q;
  ret->vm = v;

  return ret;

failed:
  if (ret)
    {
      i_free (ret);
    }
  if (p)
    {
      err_t_log_swallow (pgr_close (p, &_e), _e);
    }
  if (q)
    {
      query_provider_free (q);
    }
  if (v)
    {
      vm_close (v);
    }
  return NULL;
}

void
db_close (database *d)
{
  database_assert (d);
  err_t_log_swallow (pgr_close (d->pager, &_e), _e);
  query_provider_free (d->qspce);
}

err_t
db_execute (database *db, query *q, error *e)
{
  database_assert (db);
  return vm_execute_one_query (db->vm, q, e);
}
