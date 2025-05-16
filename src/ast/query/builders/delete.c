#include "ast/query/builders/delete.h"

#include "ast/query/queries/delete.h"
#include "dev/assert.h"
#include "errors/error.h"

DEFINE_DBG_ASSERT_I (delete_builder, delete_builder, c)
{
  ASSERT (c);
}

delete_builder
dltb_create (void)
{
  delete_builder ret = {
    .vname = { 0 },
    .got_vname = false,
  };

  delete_builder_assert (&ret);

  return ret;
}

err_t
dltb_accept_string (delete_builder *b, const string vname, error *e)
{
  delete_builder_assert (b);
  if (b->got_vname)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Delete Builder: "
          "Already have a variable name");
    }

  if (vname.len == 0)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Delete Builder: "
          "Length of variable names must be > 0");
    }

  b->vname = vname;
  b->got_vname = true;

  return SUCCESS;
}

err_t
dltb_build (delete_query *dest, delete_builder *b, error *e)
{
  delete_builder_assert (b);
  ASSERT (dest);

  if (!b->got_vname)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Delete Builder: "
          "Need a variable name to build a delete query");
    }

  dest->vname = b->vname;

  return SUCCESS;
}
