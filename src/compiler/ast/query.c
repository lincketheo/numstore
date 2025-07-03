#include "compiler/ast/query.h"

#include "core/dev/assert.h"
#include "core/dev/testing.h"
#include "core/intf/io.h"
#include "core/intf/logging.h"
#include "core/utils/strings.h"

/////////////////////////////
//// CREATE

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

  char *str = i_malloc (n + 1, 1);
  if (str != NULL)
    {
      type_snprintf (str, n + 1, &q->type);
      i_log_info ("Variable Type: %.*s\n", n, str);
      i_free (str);
    }
  else
    {
      i_log_info ("Variable Type: (Omitted due to length)\n");
    }
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
          "Already have a type literal");
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

/////////////////////////////
//// DELETE

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

/////////////////////////////
//// INSERT

DEFINE_DBG_ASSERT_I (insert_query, insert_query, i)
{
  ASSERT (i);
}

void
i_log_insert (insert_query *q)
{
  insert_query_assert (q);
  i_log_info ("Inserting to: %.*s at index: %llu. Value:\n", q->vname.len, q->vname.data, q->start);
  i_log_literal (&q->val);
}

bool
insert_query_equal (const insert_query *left, const insert_query *right)
{
  if (!string_equal (left->vname, right->vname))
    {
      return false;
    }
  if (!literal_equal (&left->val, &right->val))
    {
      return false;
    }
  if (left->start != right->start)
    {
      return false;
    }
  return true;
}

DEFINE_DBG_ASSERT_I (insert_builder, insert_builder, c)
{
  ASSERT (c);
}

insert_builder
inb_create (void)
{
  insert_builder ret = {
    .vname = (string){ 0 },
    .val = (literal){ 0 },
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
inb_accept_literal (insert_builder *b, literal t, error *e)
{
  insert_builder_assert (b);
  if (b->got_val)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Insert Builder: "
          "Already have a literal");
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
          "Need a literal to build a insert query");
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

/////////////////////////////
//// QUERY
void
i_log_query (query *q)
{
  switch (q->type)
    {
    case QT_CREATE:
      {
        i_log_create (&q->create);
        break;
      }
    case QT_DELETE:
      {
        i_log_delete (&q->delete);
        break;
      }
    case QT_INSERT:
      {
        i_log_insert (&q->insert);
        break;
      }
    }
}

bool
query_equal (const query *left, const query *right)
{
  if (left->type != right->type)
    {
      return false;
    }

  switch (left->type)
    {
    case QT_CREATE:
      {
        return create_query_equal (&left->create, &right->create);
      }
    case QT_DELETE:
      {
        return delete_query_equal (&left->delete, &right->delete);
      }
    case QT_INSERT:
      {
        return insert_query_equal (&left->insert, &right->insert);
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}
