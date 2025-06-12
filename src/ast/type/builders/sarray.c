#include "ast/type/builders/sarray.h"

#include "dev/assert.h" // DEFINE_DBG_ASSERT_I

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
