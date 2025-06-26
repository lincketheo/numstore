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
