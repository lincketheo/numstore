#include "ast/query/query.h"

#include "ast/query/queries/create.h" // i_log_create
#include "ast/query/queries/delete.h" // i_log_delete

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
