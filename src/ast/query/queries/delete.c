#include "ast/query/queries/delete.h"

#include "dev/assert.h"
#include "intf/logging.h"
#include "mm/lalloc.h"

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

void
delete_query_create (delete_query *q)
{
  ASSERT (q);
  q->vname_allocator = lalloc_create_from (q->_vname);
  q->vname = (string){ 0 };
}

void
delete_query_reset (delete_query *q)
{
  delete_query_assert (q);
  lalloc_reset (&q->vname_allocator);
  q->vname = (string){ 0 };
}
