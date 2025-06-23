#include "compiler/ast/type/builders/sarray.h"

#include "core/dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "core/dev/testing.h" // TEST

DEFINE_DBG_ASSERT_I (sarray_builder, sarray_builder, s)
{
  ASSERT (s);
}

static const char *TAG = "Strict Array Builder";

sarray_builder
sab_create (lalloc *alloc, lalloc *dest)
{
  sarray_builder builder = {
    .head = NULL,
    .type = NULL,
    .alloc = alloc,
    .dest = dest,
  };
  return builder;
}

err_t
sab_accept_dim (sarray_builder *eb, u32 dim, error *e)
{
  sarray_builder_assert (eb);

  u16 idx = (u16)list_length (eb->head);
  llnode *slot = llnode_get_n (eb->head, idx);
  dim_llnode *node;

  if (slot)
    {
      node = container_of (slot, dim_llnode, link);
    }
  else
    {
      node = lmalloc (eb->alloc, 1, sizeof *node);
      if (!node)
        {
          return error_causef (
              e, ERR_NOMEM,
              "%s: allocation failed", TAG);
        }
      llnode_init (&node->link);
      if (!eb->head)
        {
          eb->head = &node->link;
        }
      else
        {
          list_append (&eb->head, &node->link);
        }
    }

  node->dim = dim;
  return SUCCESS;
}

err_t
sab_accept_type (sarray_builder *eb, type t, error *e)
{
  sarray_builder_assert (eb);

  if (eb->type)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: type already set", TAG);
    }

  type *tp = lmalloc (eb->alloc, 1, sizeof *tp);
  if (!tp)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s: allocation failed", TAG);
    }

  *tp = t;
  eb->type = tp;
  return SUCCESS;
}

err_t
sab_build (sarray_t *dest, sarray_builder *eb, error *e)
{
  sarray_builder_assert (eb);
  ASSERT (dest);

  if (!eb->type)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: type not set", TAG);
    }

  u16 rank = (u16)list_length (eb->head);
  if (rank == 0)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: no dims to build", TAG);
    }

  u32 *dims = lmalloc (eb->dest, rank, sizeof *dims);
  if (!dims)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s: failed to allocate dims array", TAG);
    }

  u16 i = 0;
  for (llnode *it = eb->head; it; it = it->next)
    {
      dim_llnode *dn = container_of (it, dim_llnode, link);
      dims[i++] = dn->dim;
    }

  dest->rank = rank;
  dest->dims = dims;
  dest->t = eb->type;

  return SUCCESS;
}

/* --------------------------------------------------------------------------
 * Unit tests for sarray_builder
 * Uses the project test helpers:
 *    – test_assert_int_equal(actual, expected)
 *    – test_fail_if(expr)
 *    – test_fail_if_null(expr)
 * -------------------------------------------------------------------------- */

TEST (sarray_builder)
{
  error err = error_create (NULL);
  u8 _alloc[2048];
  u8 _dest[2048];

  /* provide two fixed‑size allocators for nodes + dims array */
  lalloc alloc = lalloc_create_from (_alloc);
  lalloc dest = lalloc_create_from (_dest);

  /* 0. freshly‑created builder must be clean */
  sarray_builder sb = sab_create (&alloc, &dest);
  test_fail_if (sb.head != NULL);
  test_fail_if (sb.type != NULL);

  // 1. build without type → ERR_INVALID_ARGUMENT
  sarray_t sar = { 0 };
  test_assert_int_equal (sab_build (&sar, &sb, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 2. set type but no dims → still ERR_INVALID_ARGUMENT
  type t_u32 = (type){ .type = T_PRIM, .p = U32 };
  test_assert_int_equal (sab_accept_type (&sb, t_u32, &err), SUCCESS);
  test_assert_int_equal (sab_build (&sar, &sb, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 3. duplicate type must fail
  test_assert_int_equal (sab_accept_type (&sb, t_u32, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 4. accept first dim 10
  test_assert_int_equal (sab_accept_dim (&sb, 10, &err), SUCCESS);

  // 5. successful build now that we have type and one dim
  test_assert_int_equal (sab_build (&sar, &sb, &err), SUCCESS);
  test_assert_int_equal (sar.rank, 1);
  test_fail_if_null (sar.dims);
  test_fail_if_null (sar.t);
  test_assert_int_equal (*sar.dims, 10);
  test_assert_int_equal (sar.t->p, t_u32.p);

  // 6. accept additional dims and rebuild (rank 3)
  test_assert_int_equal (sab_accept_dim (&sb, 4, &err), SUCCESS);
  test_assert_int_equal (sab_accept_dim (&sb, 2, &err), SUCCESS);
  test_assert_int_equal (sab_build (&sar, &sb, &err), SUCCESS);
  test_assert_int_equal (sar.rank, 3);
  test_assert_int_equal (sar.dims[0], 10);
  test_assert_int_equal (sar.dims[1], 4);
  test_assert_int_equal (sar.dims[2], 2);
}
