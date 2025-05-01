#include "vm/execute.h"
#include "query/query.h"
#include "vm/create/create_execute.h"

void
query_execute (query q)
{
  switch (q.type)
    {
    case QT_CREATE:
      create_execute (q.cargs);
      break;
    default:
      panic ();
    }
}
