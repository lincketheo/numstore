#include "ast/query/builders/delete.h"

#include "ast/query/queries/delete.h" // delete
#include "dev/assert.h"               // ASSERT
#include "dev/testing.h"              // TEST
#include "errors/error.h"             // err_t

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

TEST (delete_builder)
{
  error err = error_create (NULL);

  /* 0. freshly‑created builder must be clean */
  delete_builder b = dltb_create ();
  test_fail_if (b.got_vname);

  // 1. rejecting empty variable names
  test_assert_int_equal (dltb_accept_string (&b, (string){ 0 }, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 2. first, accept a variable name
  string name = unsafe_cstrfrom ("foo");
  test_assert_int_equal (dltb_accept_string (&b, name, &err), SUCCESS);
  test_fail_if (!b.got_vname);

  // 3. duplicate name must fail
  test_assert_int_equal (dltb_accept_string (&b, name, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 4. build now that the piece is present
  delete_query q = { 0 };
  test_assert_int_equal (dltb_build (&q, &b, &err), SUCCESS);
  test_fail_if (q.vname.len != name.len);

  // 5. build with *missing vname* must fail
  delete_builder missing_name = dltb_create ();
  test_assert_int_equal (dltb_build (&q, &missing_name, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;
}
