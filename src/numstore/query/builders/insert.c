#include "numstore/query/builders/insert.h"

#include "core/dev/assert.h"   // DEFINE_DBG_ASSERT_I
#include "core/dev/testing.h"  // TEST
#include "core/ds/strings.h"   // TODO
#include "core/errors/error.h" // err_t

DEFINE_DBG_ASSERT_I (insert_builder, insert_builder, c)
{
  ASSERT (c);
}

insert_builder
inb_create (void)
{
  insert_builder ret = {
    .vname = (string){ 0 },
    .val = (value){ 0 },
    .start = 0,

    .got_val = false,
    .got_vname = false,
    .got_start = false,
  };

  insert_builder_assert (&ret);

  return ret;
}

err_t
inb_accept_string (insert_builder *b, const string vname, error *e)
{
  insert_builder_assert (b);
  if (b->got_vname)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Insert Builder: "
          "Already have a variable name");
    }

  if (vname.len == 0)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Insert Builder: "
          "Length of variable names must be > 0");
    }

  b->vname = vname;
  b->got_vname = true;

  return SUCCESS;
}

err_t
inb_accept_value (insert_builder *b, value t, error *e)
{
  insert_builder_assert (b);
  if (b->got_val)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Insert Builder: "
          "Already have a value");
    }

  b->val = t;
  b->got_val = true;

  return SUCCESS;
}

err_t
inb_accept_start (insert_builder *b, b_size start, error *e)
{
  insert_builder_assert (b);
  if (b->got_start)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Insert Builder: "
          "Already have a start index");
    }

  b->start = start;
  b->got_start = true;

  return SUCCESS;
}

err_t
inb_build (insert_query *dest, insert_builder *b, error *e)
{
  insert_builder_assert (b);
  ASSERT (dest);

  if (!b->got_val)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Insert Builder: "
          "Need a value to build a insert query");
    }

  if (!b->got_vname)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Insert Builder: "
          "Need a variable name to build a insert query");
    }

  dest->val = b->val;
  dest->vname = b->vname;
  dest->start = b->start;

  return SUCCESS;
}
