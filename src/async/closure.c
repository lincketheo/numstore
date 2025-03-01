#include "async/closure.h"

void
clsr_execute (closure *cl)
{
  closure_assert (cl);
  cl->func (cl->context);
}
