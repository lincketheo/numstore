#include "vm/vm.h"
#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "query/query.h"
#include "vm/execute.h"

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
}

void
vm_execute (vm *v)
{
  vm_assert (v);

  query q;
  u32 read = cbuffer_read (&q, sizeof q, 1, v->queries_input);
  ASSERT (read == 0 || read == 1);

  if (read > 0)
    {
      query_execute (q);
    }
}
