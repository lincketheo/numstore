#include "vm/vm.h"
#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "query/queries/create.h"
#include "query/query.h"

DEFINE_DBG_ASSERT_I (vm, vm, v)
{
  ASSERT (v);
  ASSERT (v->queries_input);
}

vm
vm_create (vm_params args)
{
  ASSERT (args.queries_input->cap % sizeof (query) == 0);

  vm ret = {
    .queries_input = args.queries_input,
  };

  vm_assert (&ret);

  return ret;
}

void
vm_execute (vm *v)
{
  vm_assert (v);

  query q;
  u32 read = cbuffer_read (&q, sizeof q, 1, v->queries_input);

  if (read > 0)
    {
      switch (q.type)
        {
        case QT_CREATE:
          {
            i_log_create (&q.cquery);
            break;
          }
        case QT_DELETE:
          {
            i_log_delete (&q.dquery);
            break;
          }
        default:
          {
            UNREACHABLE ();
          }
        }
    }
}
