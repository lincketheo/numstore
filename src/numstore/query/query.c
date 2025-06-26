#include "numstore/query/query.h"

#include "core/intf/logging.h" // TODO

#include "numstore/query/queries/create.h" // i_log_create
#include "numstore/query/queries/delete.h" // i_log_delete
#include "numstore/query/queries/insert.h"

void
i_log_query (query q)
{
  switch (q.type)
    {
    case QT_CREATE:
      {
        i_log_create (q.create);
        break;
      }
    case QT_DELETE:
      {
        i_log_delete (q.delete);
        break;
      }
    case QT_INSERT:
      {
        i_log_insert (q.insert);
        break;
      }
    }
}

struct query_s
query_error_create (error e)
{
  return (query){
    .ok = false,
    .e = e,
  };
}

bool
query_equal (const query *left, const query *right)
{
  if (left->type != right->type)
    {
      return false;
    }

  switch (left->type)
    {
    case QT_CREATE:
      {
        return create_query_equal (left->create, right->create);
      }
    case QT_DELETE:
      {
        return delete_query_equal (left->delete, right->delete);
      }
    case QT_INSERT:
      {
        return insert_query_equal (left->insert, right->insert);
      }
    }
}
