#include "core/dev/assert.h"   // TODO
#include "core/dev/testing.h"  // TODO
#include "core/errors/error.h" // TODO
#include "core/intf/io.h"      // TODO
#include "core/intf/stdlib.h"  // TODO
#include "core/mm/lalloc.h"    // TODO

#include "compiler/ast/query/queries/create.h" // TODO
#include "compiler/ast/query/queries/delete.h" // TODO
#include "compiler/ast/query/query.h"          // TODO
#include "compiler/ast/query/query_provider.h" // TODO

DEFINE_DBG_ASSERT_I (query_provider, query_provider, q)
{
  ASSERT (q);
}

query_provider *
query_provider_create (error *e)
{
  query_provider *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "Not enough memory to allocate "
          "query_provider");
      return NULL;
    }

  // set is_used = false
  i_memset (ret->create, 0, sizeof (ret->create));
  i_memset (ret->delete, 0, sizeof (ret->delete));

  query_provider_assert (ret);

  return ret;
}

err_t
query_provider_get (
    query_provider *q,
    query *dest,
    query_t type,
    error *e)
{
  query_provider_assert (q);
  ASSERT (dest);

  for (u32 i = 0; i < 20; ++i)
    {
      switch (type)
        {
        case QT_CREATE:
          {
            if (!q->create[i].is_used)
              {
                *dest = create_query_create (&q->create[i].create);
                q->create[i].is_used = true;
                return SUCCESS;
              }
            break;
          }
        case QT_DELETE:
          {
            if (!q->delete[i].is_used)
              {
                *dest = delete_query_create (&q->delete[i].delete);
                q->delete[i].is_used = true;
                return SUCCESS;
              }
            break;
          }
        }
    }

  return error_causef (e, ERR_NOMEM, "Query provider full");
}

void
query_provider_release (query_provider *q, query *qu)
{
  query_provider_assert (q);
  ASSERT (qu);

  for (u32 i = 0; i < 20; ++i)
    {
      switch (qu->type)
        {
        case QT_CREATE:
          {
            // POINTER COMPARISON
            // Maybe id's instead of pointers?
            if (&q->create[i].create == qu->create)
              {
                ASSERT (q->create[i].is_used);
                q->create[i].is_used = false;
                return;
              }
            break;
          }
        case QT_DELETE:
          {
            if (&q->delete[i].delete == qu->delete)
              {
                ASSERT (q->delete[i].is_used);
                q->delete[i].is_used = false;
                return;
              }
            break;
          }
        default:
          {
            UNREACHABLE ();
          }
        }
    }
}

void
query_provider_free (query_provider *q)
{
  query_provider_assert (q);
  i_free (q);
}

TEST (query_provider)
{
  error err = error_create (NULL);

  /* 0. create and basic sanity */
  query_provider *qp = query_provider_create (&err);
  test_fail_if_null (qp);

  // 1. first QT_CREATE slot should be available
  query q = { 0 };
  test_assert_int_equal (query_provider_get (qp, &q, QT_CREATE, &err), SUCCESS);
  test_assert_int_equal (q.type, QT_CREATE);
  test_fail_if_null (q.create);

  // 2. release that slot and grab again (should recycle)
  query_provider_release (qp, &q);
  test_assert_int_equal (query_provider_get (qp, &q, QT_CREATE, &err), SUCCESS);
  test_assert_int_equal (q.type, QT_CREATE);
  query_provider_release (qp, &q);

  // 3. fill all 20 CREATE slots, next one must fail with ERR_NOMEM
  query create_slots[20] = { 0 };
  for (u32 i = 0; i < 20; ++i)
    {
      test_assert_int_equal (query_provider_get (qp, &create_slots[i], QT_CREATE, &err), SUCCESS);
    }
  test_assert_int_equal (query_provider_get (qp, &q, QT_CREATE, &err), ERR_NOMEM);
  err.cause_code = SUCCESS;

  // 4. release one slot, then get succeeds again
  query_provider_release (qp, &create_slots[0]);
  test_assert_int_equal (query_provider_get (qp, &q, QT_CREATE, &err), SUCCESS);

  // 5. same capacity test for DELETE queries
  query delete_slots[20] = { 0 };
  for (u32 i = 0; i < 20; ++i)
    {
      test_assert_int_equal (query_provider_get (qp, &delete_slots[i], QT_DELETE, &err), SUCCESS);
    }
  test_assert_int_equal (query_provider_get (qp, &q, QT_DELETE, &err), ERR_NOMEM);
  err.cause_code = SUCCESS;

  // 6. mixed release and re‑acquire for DELETE
  query_provider_release (qp, &delete_slots[5]);
  test_assert_int_equal (query_provider_get (qp, &q, QT_DELETE, &err), SUCCESS);

  query_provider_free (qp);
}
