#include "compiler/ast/query/queries/delete.h"

#include "compiler/ast/query/query.h" // query
#include "core/dev/assert.h"          // DEFINE_DBG_ASSERT_I
#include "core/intf/logging.h"        // i_log_info

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

query
delete_query_create (delete_query *q)
{
  ASSERT (q);

  q->query_allocator = lalloc_create_from (q->_query_space);
  q->vname = (string){ 0 };

  delete_query_assert (q);

  return (query){
    .type = QT_DELETE,
    .qalloc = &q->query_allocator,
    .delete = q,
    .ok = true,
    .e = error_create (NULL),
  };
}

void
delete_query_reset (delete_query *q)
{
  delete_query_assert (q);
  lalloc_reset (&q->query_allocator);
  q->vname = (string){ 0 };
}
