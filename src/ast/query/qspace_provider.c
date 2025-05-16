#include "ast/query/qspace_provider.h"
#include "ast/query/queries/create.h"
#include "ast/query/queries/delete.h"
#include "ast/query/query.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "intf/stdlib.h"
#include "mm/lalloc.h"

#include <stdlib.h>

qspce_prvdr *
qspce_prvdr_create (void)
{
  qspce_prvdr *ret = malloc (sizeof *ret);
  if (ret == NULL)
    {
      return NULL;
    }

  for (u32 i = 0; i < 20; ++i)
    {
      ret->create[i] = (create_wrapper){
        .is_used = false,
        .query = (query){
            .alloc = lalloc_create_from (&ret->create[i].create.data),
            .create = &ret->create[i].create,
            .type = QT_CREATE,
        },
      };

      ret->delete[i] = (delete_wrapper){
        .is_used = false,
        .query = (query){
            .alloc = lalloc_create_from (&ret->delete[i].delete.data),
            .delete = &ret->delete[i].delete,
            .type = QT_DELETE,
        },
      };
    }

  return ret;
}

err_t
qspce_prvdr_get (
    qspce_prvdr *q,
    query **dest,
    query_t type,
    error *e)
{
  ASSERT (q);
  ASSERT (dest);

  for (u32 i = 0; i < 20; ++i)
    {
      switch (type)
        {
        case QT_CREATE:
          {
            if (!q->create[i].is_used)
              {
                // reset lalloc
                q->create[i].query.alloc = lalloc_create_from (q->create[i].create.data);
                *dest = &q->create[i].query;
                q->create[i].is_used = true;
                return SUCCESS;
              }
            break;
          }
        case QT_DELETE:
          {
            if (!q->delete[i].is_used)
              {
                // reset lalloc
                q->delete[i].query.alloc = lalloc_create_from (q->delete[i].delete.data);
                *dest = &q->delete[i].query;
                q->delete[i].is_used = true;
                return SUCCESS;
              }
            break;
          }
        }
    }

  *dest = NULL;
  return error_causef (
      e, ERR_NOMEM,
      "Too many query spaces being used");
}

void
qspce_prvdr_release (qspce_prvdr *q, query *qu, query_t type)
{
  for (u32 i = 0; i < 20; ++i)
    {
      switch (type)
        {
        case QT_CREATE:
          {
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
        }
    }
}

void
qspce_prvdr_free (qspce_prvdr *q)
{
  ASSERT (q);
  free (q);
}
