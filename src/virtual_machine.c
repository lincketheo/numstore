#include "virtual_machine.h"

#include "ast/query/queries/delete.h"
#include "dev/assert.h" // ASSERT

static inline err_t
create_query_execute (pager *p, create_query *q, error *e)
{
  (void)p;
  (void)e;
  i_log_create (q);
  return SUCCESS;
  // cursor c = crsr_open (p);
  // return crsr_create_var (&c, q, e);
}

static inline err_t
delete_query_execute (pager *p, delete_query *q, error *e)
{
  (void)p;
  (void)e;
  i_log_delete (q);
  return SUCCESS;
  // cursor c = crsr_open (p);
  // return crsr_create_var (&c, q, e);
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
        return delete_query_execute (p, q->delete, e);
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}
