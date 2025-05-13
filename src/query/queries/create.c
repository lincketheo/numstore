#include "query/queries/create.h"
#include "dev/assert.h"
#include "intf/logging.h"
#include "type/types.h"

#include <stdlib.h>

DEFINE_DBG_ASSERT_I (create_query, create_query, q)
{
  ASSERT (q);
}

void
i_log_create (create_query *q)
{
  create_query_assert (q);
  int n = type_snprintf (NULL, 0, &q->type);
  char *str = malloc (n + 1);
  type_snprintf (str, n, &q->type);
  i_log_info ("create \n%.*s\n%.*s\n",
              q->vname.len, q->vname.data,
              n, str);
  free (str);
}
