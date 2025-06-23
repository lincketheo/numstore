#include "compiler/ast/query/query.h"

#include "compiler/ast/query/queries/create.h" // i_log_create
#include "compiler/ast/query/queries/delete.h" // i_log_delete
#include "core/intf/logging.h"

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
