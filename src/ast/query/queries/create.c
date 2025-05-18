#include "ast/query/queries/create.h"
#include "ast/type/types.h"
#include "dev/assert.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "mm/lalloc.h"

DEFINE_DBG_ASSERT_I (create_query, create_query, q)
{
  ASSERT (q);
}

void
i_log_create (create_query *q)
{
  create_query_assert (q);
  int n = type_snprintf (NULL, 0, &q->type);
  char *str = i_malloc (n + 1, sizeof *str);
  type_snprintf (str, n, &q->type);
  i_log_info ("create \n%.*s\n%.*s\n",
              q->vname.len, q->vname.data,
              n, str);
  i_free (str);
}

void
create_query_create (create_query *q)
{
  ASSERT (q);
  q->alloc = lalloc_create_from (q->data);
  q->vname = (string){ 0 };
  q->type = (type){ 0 };
  create_query_assert (q);
}

void
create_query_reset (create_query *q)
{
  create_query_assert (q);
  lalloc_reset (&q->alloc);
}
