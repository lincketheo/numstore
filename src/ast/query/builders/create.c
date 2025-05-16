#include "ast/query/builders/create.h"

#include "dev/assert.h"
#include "errors/error.h"

DEFINE_DBG_ASSERT_I (create_builder, create_builder, c)
{
  ASSERT (c);
}

create_builder
crb_create (void)
{
  create_builder ret = {
    .type = (type){ 0 },
    .vname = (string){ 0 },
    .got_type = false,
    .got_vname = false,
  };

  create_builder_assert (&ret);

  return ret;
}

err_t
crb_accept_string (create_builder *b, const string vname, error *e)
{
  create_builder_assert (b);
  if (b->got_vname)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Create Builder: "
          "Already have a variable name");
    }

  if (vname.len == 0)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Create Builder: "
          "Length of variable names must be > 0");
    }

  b->vname = vname;
  b->got_vname = true;

  return SUCCESS;
}

err_t
crb_accept_type (create_builder *b, type t, error *e)
{
  create_builder_assert (b);
  if (b->got_type)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Create Builder: "
          "Already have a type value");
    }

  b->type = t;
  b->got_type = true;

  return SUCCESS;
}

err_t
crb_build (create_query *dest, create_builder *b, error *e)
{
  create_builder_assert (b);
  ASSERT (dest);

  if (!b->got_type)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Create Builder: "
          "Need a type to build a create query");
    }

  if (!b->got_vname)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Create Builder: "
          "Need a variable name to build a create query");
    }

  dest->type = b->type;
  dest->vname = b->vname;

  return SUCCESS;
}
