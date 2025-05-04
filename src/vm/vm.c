#include "vm/vm.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/cbuffer.h"
#include "query/query.h"
#include "services/var_create.h"

DEFINE_DBG_ASSERT_I (vm, vm, v)
{
  ASSERT (v);
  ASSERT (v->queries_input);
}

void
vm_create (vm *dest, vm_params args)
{
  ASSERT (dest);
  ASSERT (args.queries_input->cap % sizeof (query) == 0);
  dest->queries_input = args.queries_input;
  dest->services = args.services;
}

void
vm_execute (vm *v)
{
  vm_assert (v);

  err_t ret = SUCCESS;

  query q;
  u32 read = cbuffer_read (&q, sizeof q, 1, v->queries_input);
  ASSERT (read == 0 || read == 1);

  if (read > 0)
    {
      switch (q.type)
        {
        case QT_CREATE:
          {
            if ((ret = var_create_create_var (
                     &v->services->vcreate, q.cquery)))
              {
                panic ();
              }
            return;
          }
        default:
          {
            panic ();
          }
        }
    }
}
