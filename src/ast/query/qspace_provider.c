#include "ast/query/qspace_provider.h"
#include "ast/query/queries/create.h"
#include "ast/query/queries/delete.h"
#include "ast/query/query.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/stdlib.h"
#include "mm/lalloc.h"

DEFINE_DBG_ASSERT_I (qspce_prvdr, qspce_prvdr, q)
{
  ASSERT (q);
}

qspce_prvdr *
qspce_prvdr_create (error *e)
{
  qspce_prvdr *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "Not enough memory to allocate "
          "qspce_prvdr");
      return NULL;
    }

  for (u32 i = 0; i < 20; ++i)
    {
      // Create
      create_query_create (&ret->create[i].create);
      ret->create[i].is_used = false;

      // Delete
      delete_query_create (&ret->delete[i].delete);
      ret->delete[i].is_used = false;
    }

  qspce_prvdr_assert (ret);

  return ret;
}

err_t
qspce_prvdr_get (
    qspce_prvdr *q,
    query *dest,
    query_t type,
    error *e)
{
  qspce_prvdr_assert (q);
  ASSERT (dest);

  for (u32 i = 0; i < 20; ++i)
    {
      switch (type)
        {
        case QT_CREATE:
          {
            if (!q->create[i].is_used)
              {
                create_query_reset (&q->create[i].create);
                q->create[i].is_used = true;
                *dest = (query){
                  .type = type,
                  .create = &q->create[i].create,
                  .qalloc = &q->create[i].create.alloc,
                };
                return SUCCESS;
              }
            break;
          }
        case QT_DELETE:
          {
            if (!q->delete[i].is_used)
              {
                delete_query_reset (&q->delete[i].delete);
                q->delete[i].is_used = true;
                *dest = (query){
                  .type = type,
                  .delete = &q->delete[i].delete,
                  .qalloc = &q->delete[i].delete.vname_allocator,
                };
                return SUCCESS;
              }
            break;
          }
        }
    }

  return error_causef (e, ERR_NOMEM, "Query provider full");
}

void
qspce_prvdr_release (qspce_prvdr *q, query *qu)
{
  qspce_prvdr_assert (q);
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
qspce_prvdr_free (qspce_prvdr *q)
{
  qspce_prvdr_assert (q);
  i_free (q);
}
