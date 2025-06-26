#include "numstore/query/queries/create.h"

#include "core/dev/assert.h"   // DEFINE_DBG_ASSERT_I
#include "core/intf/logging.h" // i_log_info

#include "numstore/query/query.h" // query

DEFINE_DBG_ASSERT_I (create_query, create_query, q)
{
  ASSERT (q);
}

void
i_log_create (create_query *q)
{
  create_query_assert (q);
  int n = type_snprintf (NULL, 0, &q->type);

  i_log_info ("Creating variable with name: %.*s\n",
              q->vname.len, q->vname.data);

  // We're just using the query_allocator as a temporary store
  // for the variable name if we can fit it, otherwise, just
  // omit it - not the cleanest
  u32 state = lalloc_get_state (&q->query_allocator);
  char *str = lmalloc (&q->query_allocator, n + 1, 1);
  if (str != NULL)
    {
      type_snprintf (str, n + 1, &q->type);
      i_log_info ("Variable Type: %.*s\n", n, str);
      lalloc_reset_to_state (&q->query_allocator, state);
    }
  else
    {
      i_log_info ("Variable Type: (Omitted due to length)");
    }
}

query
create_query_create (create_query *q)
{
  ASSERT (q);

  q->query_allocator = lalloc_create_from (q->_query_space);
  q->vname = (string){ 0 };
  q->type = (type){ 0 };

  create_query_assert (q);

  return (query){
    .type = QT_CREATE,
    .qalloc = &q->query_allocator,
    .create = q,
    .ok = true,
    .e = error_create (NULL),
  };
}

void
create_query_reset (create_query *q)
{
  create_query_assert (q);
  lalloc_reset (&q->query_allocator);
}

bool
create_query_equal (const create_query *left, const create_query *right)
{
  create_query_assert (left);
  create_query_assert (right);

  if (!string_equal (left->vname, right->vname))
    {
      return false;
    }
  if (!type_equal (&left->type, &right->type))
    {
      return false;
    }
  return true;
}
