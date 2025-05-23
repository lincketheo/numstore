#include "ast/query/query_provider.h"

#include "ast/query/queries/create.h"
#include "ast/query/queries/delete.h"
#include "ast/query/query.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/stdlib.h"
#include "mm/lalloc.h"

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
