#include "query/queries/delete.h"
#include "dev/assert.h"
#include "intf/logging.h"
#include "type/types.h"

#include <stdlib.h>

DEFINE_DBG_ASSERT_I (delete_query, delete_query, q)
{
  ASSERT (q);
}

void
i_log_delete (delete_query *q)
{
  delete_query_assert (q);
  i_log_info ("delete %.*s\n", q->vname.len, q->vname.data);
}
