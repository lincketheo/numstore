#include "virtual_machine.h"

#include "cursor/cursor.h"
#include "dev/assert.h" // ASSERT

static inline err_t
create_query_execute (pager *p, create_query *q, error *e)
{
  cursor c = crsr_open (p);
  return crsr_create_var (&c, q, e);
}

err_t
query_execute (pager *p, query *q, error *e)
{
  ASSERT (q);

  switch (q->type)
    {
    case QT_CREATE:
      {
        return create_query_execute (p, q->create, e);
      }
    case QT_DELETE:
      {
        panic ();
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}
