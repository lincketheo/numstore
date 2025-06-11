#include "virtual_machine.h"

#include "ast/query/queries/delete.h"
#include "cursor/cursor.h"
#include "dev/assert.h" // ASSERT
#include "errors/error.h"

struct vm_s
{
  cursor *c;
};

DEFINE_DBG_ASSERT_I (vm, vm, v)
{
  ASSERT (v);
  ASSERT (v->c);
}

static const char *TAG = "Virtual Machine";

vm *
vm_open (pager *p, error *e)
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

  vm_assert (ret);

  return ret;
}

void
vm_close (vm *v)
{
  vm_assert (v);
}

static inline err_t
create_query_execute (vm *v, create_query *q, error *e)
{
  i_log_create (q);
  return cursor_create_var (v->c, q, e);
}

static inline err_t
delete_query_execute (vm *v, delete_query *q, error *e)
{
  (void)v;
  (void)e;
  i_log_delete (q);
  return SUCCESS;
}

err_t
vm_execute_one_query (vm *v, query *q, error *e)
{
  ASSERT (q);

  switch (q->type)
    {
    case QT_CREATE:
      {
        return create_query_execute (v, q->create, e);
      }
    case QT_DELETE:
      {
        return delete_query_execute (v, q->delete, e);
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}
