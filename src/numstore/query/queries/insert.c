#include "numstore/query/queries/insert.h"
#include "compiler/value/value.h"
#include "core/dev/assert.h"
#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "core/intf/logging.h"
#include "core/mm/lalloc.h"
#include "core/utils/strings.h" // TODO
#include "numstore/query/query.h"

DEFINE_DBG_ASSERT_I (insert_query, insert_query, i)
{
  ASSERT (i);
}

void
i_log_insert (insert_query *q)
{
  insert_query_assert (q);
  i_log_info ("Inserting to: %.*s at index: %llu. Value:\n", q->vname.len, q->vname.data, q->start);
  i_log_value (&q->val);
}

query
insert_query_create (insert_query *q)
{
  ASSERT (q);
  q->query_allocator = lalloc_create_from (q->_query_space);
  q->vname = (string){ 0 };
  q->val = (value){ 0 };
  q->start = 0;

  insert_query_assert (q);

  return (query){
    .type = QT_INSERT,
    .qalloc = &q->query_allocator,
    .insert = q,
    .ok = true,
    .e = error_create (NULL),
  };
}

void
insert_query_reset (insert_query *q)
{
  insert_query_assert (q);
  lalloc_reset (&q->query_allocator);
}

bool
insert_query_equal (const insert_query *left, const insert_query *right)
{
  if (!string_equal (left->vname, right->vname))
    {
      return false;
    }
  if (!value_equal (&left->val, &right->val))
    {
      return false;
    }
  if (left->start != right->start)
    {
      return false;
    }
  return true;
}
