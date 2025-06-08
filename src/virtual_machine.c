#include "virtual_machine.h"

#include "ast/query/queries/delete.h"
#include "cursor/cursor.h"
#include "dev/assert.h" // ASSERT
#include "errors/error.h"

struct vm_s
{
  cursor *c;
  cbuffer *query_input;
  cbuffer *output;
};

static const char *TAG = "Virtual Machine";

vm *
vm_create (pager *p, error *e)
{
  vm *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "%s Failed to allocate virtual machine", TAG);
      return NULL;
    }

  cursor *c = cursor_open (p, e);
  if (c == NULL)
    {
      return NULL;
    }

  ret->c = c;
  return ret;
}

/**
static inline err_t
create_query_execute (pager *p, create_query *q, error *e)
{
  i_log_create (q);

  cursor c = crsr_open (p);
  if (crsr_create_var (&c, q, e))
    {
      goto theend;
    }

theend:
  return err_t_from (e);
}
*/

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
        panic ();
        // return create_query_execute (p, q->create, e);
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
