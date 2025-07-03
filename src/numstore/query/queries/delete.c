#include "numstore/query/queries/delete.h"

#include "core/dev/assert.h"    // DEFINE_DBG_ASSERT_I
#include "core/intf/logging.h"  // i_log_info
#include "core/utils/strings.h" // TODO

#include "numstore/query/query.h" // query

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

bool
delete_query_equal (const delete_query *left, const delete_query *right)
{
  delete_query_assert (left);
  delete_query_assert (right);

  if (!string_equal (left->vname, right->vname))
    {
      return false;
    }

  return true;
}
