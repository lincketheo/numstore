#include "database.h"

#include "ast/query/query_provider.h" // qspce
#include "dev/assert.h"               // DEFINE_DBG_ASSERT_I
#include "dev/testing.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/io.h" // i_touch
#include "math/random.h"
#include "paging/pager.h"    // pager
#include "virtual_machine.h" // vm

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

TEST (db_create_var)
{
  error e = error_create (NULL);
  test_fail_if (i_remove_quiet (unsafe_cstrfrom ("testing.db"), &e));

  rand_seed ();
  u8 buf[20480];
  lalloc l = lalloc_create (buf, 2048);

  type t1, t2, t3;
  test_fail_if (type_random (&t1, &l, 5, &e)); // This can fail predictably
  test_fail_if (type_random (&t2, &l, 5, &e));
  test_fail_if (type_random (&t3, &l, 5, &e));

  database *db = db_open (unsafe_cstrfrom ("testing.db"), &e);
  test_fail_if_null (db);

  create_query c1;

  query q1 = create_query_create (&c1);

  c1.vname = (string){
    .data = "foo",
    .len = 3,
  };
  c1.type = t1;

  test_fail_if (db_execute (db, &q1, &e));

  c1.vname = (string){
    .data = "bar",
    .len = 3,
  };
  c1.type = t2;

  test_fail_if (db_execute (db, &q1, &e));

  c1.vname = (string){
    .data = "biz",
    .len = 3,
  };
  c1.type = t3;

  test_fail_if (db_execute (db, &q1, &e));

  c1.vname = (string){
    .data = "foo",
    .len = 3,
  };
  c1.type = t1;

  test_assert_int_equal (db_execute (db, &q1, &e), ERR_ALREADY_EXISTS);
  e.cause_code = SUCCESS;

  c1.vname = (string){
    .data = "bar",
    .len = 3,
  };
  c1.type = t2;

  test_assert_int_equal (db_execute (db, &q1, &e), ERR_ALREADY_EXISTS);
  e.cause_code = SUCCESS;

  c1.vname = (string){
    .data = "biz",
    .len = 3,
  };
  c1.type = t3;

  test_assert_int_equal (db_execute (db, &q1, &e), ERR_ALREADY_EXISTS);
  e.cause_code = SUCCESS;

  db_close (db);
}
