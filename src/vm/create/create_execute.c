
#include "vm/create/create_execute.h"
#include "intf/logging.h"
#include "query/queries/create.h"
#include "typing.h"

void
create_execute (create_args cargs)
{
  i_log_info ("Executing Create Query.\n");
  i_log_info ("Variable name: %.*s\n", cargs.vname.len, cargs.vname.data);
  i_log_info ("Variable type: \n");
  i_log_type (&cargs.type);
}
