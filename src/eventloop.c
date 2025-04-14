#include "eventloop.h"
#include "dev/errors.h"
#include "net.h"
#include "os/types.h"

#include <fcntl.h>

sctx s;

void
setup ()
{
  s = sctx_open ();
}

int
execute ()
{
  sctx_accept (&s);
  sctx_read_write_some (&s);
  sctx_do_work (&s);
  return SUCCESS;
}

void
teardown ()
{
}
