#include "ast/query/builders/create.h"

#include "dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "dev/testing.h" // TEST
#include "ds/strings.h"
#include "errors/error.h" // err_t

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

TEST (create_builder)
{
  error err = error_create (NULL);

  // 0. freshly‑created builder must be clean
  create_builder b = crb_create ();
  test_fail_if (b.got_vname);
  test_fail_if (b.got_type);

  // 1. rejecting empty variable names
  test_assert_int_equal (crb_accept_string (&b, (string){ 0 }, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 2. first, accept a variable name
  string name = unsafe_cstrfrom ("foo");
  test_assert_int_equal (crb_accept_string (&b, name, &err), SUCCESS);
  test_fail_if (!b.got_vname);

  // 3. duplicate name must fail
  test_assert_int_equal (crb_accept_string (&b, name, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 4. accept a type
  type t = (type){ .type = T_PRIM };
  test_assert_int_equal (crb_accept_type (&b, t, &err), SUCCESS);
  test_fail_if (!b.got_type);

  // 5. duplicate type must fail
  test_assert_int_equal (crb_accept_type (&b, t, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 6. build now that both pieces are present
  create_query q = { 0 };
  test_assert_int_equal (crb_build (&q, &b, &err), SUCCESS);
  test_fail_if (q.type.type != t.type);
  test_fail_if (q.vname.len != name.len);

  // 7. build with *missing type* must fail
  create_builder missing_type = crb_create ();
  test_assert_int_equal (crb_accept_string (&missing_type, name, &err), SUCCESS);
  test_assert_int_equal (crb_build (&q, &missing_type, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 8. build with *missing vname* must fail
  create_builder missing_name = crb_create ();
  test_assert_int_equal (crb_accept_type (&missing_name, t, &err), SUCCESS);
  test_assert_int_equal (crb_build (&q, &missing_name, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 9. order‑independence: type first, then name
  create_builder reorder = crb_create ();
  test_assert_int_equal (crb_accept_type (&reorder, t, &err), SUCCESS);
  test_assert_int_equal (crb_accept_string (&reorder, name, &err), SUCCESS);
  test_assert_int_equal (crb_build (&q, &reorder, &err), SUCCESS);
}
