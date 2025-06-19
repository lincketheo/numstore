#include "virtual_machine.h"

#include "ast/query/queries/delete.h"
#include "ast/query/query.h"
#include "cursor/cursor.h"
#include "dev/assert.h" // ASSERT
#include "ds/cbuffer.h"
#include "errors/error.h"

struct vm_s
{
  cursor *c;

  cbuffer *input;
  cbuffer output;
  char _output[256];

  u16 eidx;
  bool is_active;
  query active; // Current query
};

DEFINE_DBG_ASSERT_I (vm, vm, v)
{
  ASSERT (v);
  ASSERT (v->input);
  ASSERT (v->c);
}

static const char *TAG = "Virtual Machine";

vm *
vm_open (pager *p, cbuffer *input, error *e)
{
  vm *ret = i_malloc (1, sizeof *ret);
  ASSERT (input->cap % sizeof (query) == 0);
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

  *ret = (vm){
    .c = c,
    .input = input,
    .output = cbuffer_create_from (ret->_output),

    .is_active = false,
    // active
  };

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

cbuffer *
vm_get_output (vm *v)
{
  vm_assert (v);
  return &v->output;
}

err_t
vm_execute_one_query (vm *v, query *q, error *e)
{
  vm_assert (v);
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

void
vm_execute (vm *v)
{
  vm_assert (v);

  // Try to finish this query
  if (v->is_active)
    {
      if (!v->active.ok)
        {
          ASSERT (v->eidx < v->active.e.cmlen);
          u32 remainder = v->active.e.cmlen - v->eidx;
          u32 written = cbuffer_write (v->active.e.cause_msg, 1, remainder, &v->output);
          v->eidx += written;
          ASSERT (v->eidx <= v->active.e.cmlen);
          if (v->eidx == v->active.e.cmlen)
            {
              v->eidx = 0;
              v->is_active = false;
            }
        }
    }

  if (!v->is_active)
    {
    }
}
