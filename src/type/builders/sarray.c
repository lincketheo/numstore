#include "type/builders/sarray.h"
#include "dev/assert.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/stdlib.h"
#include "mm/lalloc.h"
#include "type/types.h"

DEFINE_DBG_ASSERT_I (sarray_builder, sarray_builder, s)
{
  ASSERT (s);
  ASSERT (s->cap > 0);
  ASSERT (s->len <= s->cap);

  ASSERT (s->dims);
  ASSERT (s->alloc);
}

err_t
sab_create (sarray_builder *dest, lalloc *alloc, error *e)
{
  ASSERT (dest);

  lalloc_r dims = lmalloc (alloc, 10, 2, sizeof *dest->dims);

  if (dims.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Strict Array Builder: Failed to allocate "
          "memory for dimension buffer");
    }

  dest->type = NULL;
  dest->dims = dims.ret;
  dest->len = 0;
  dest->cap = dims.rlen;
  dest->alloc = alloc;

  sarray_builder_assert (dest);

  return SUCCESS;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline err_t
sab_double_cap (sarray_builder *sab, error *e)
{
  sarray_builder_assert (sab);

  lalloc_r dims = lrealloc (
      sab->alloc,
      sab->dims,
      2 * sab->cap,
      sab->cap + 1,
      sizeof *sab->dims);

  if (dims.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Strict Array Builder: Failed to reallocate dimension buffer");
    }

  sab->cap = dims.rlen;
  sab->dims = dims.ret;

  return SUCCESS;
}

err_t
sab_accept_dim (sarray_builder *sab, u32 dim, error *e)
{
  sarray_builder_assert (sab);

  if (dim == 0)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Strict Array Builder: "
          "Expecting dimensions to be greater than 0");
    }

  if (sab->len == sab->cap)
    {
      err_t_wrap (sab_double_cap (sab, e), e);
    }

  sab->dims[sab->len++] = dim;

  return SUCCESS;
}

err_t
sab_accept_type (sarray_builder *sab, type t, error *e)
{
  sarray_builder_assert (sab);

  if (sab->type)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Strict Array Builder: "
          "Got multiple types");
    }

  lalloc_r type = lmalloc (sab->alloc, 1, 1, sizeof *sab->type);
  if (type.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Strict Array Builder: "
          "Not enough memory to allocate new type");
    }
  i_memcpy (type.ret, &t, sizeof *sab->type);

  sab->type = type.ret;

  return SUCCESS;
}

static inline err_t
sab_clip (sarray_builder *sab, error *e)
{
  sarray_builder_assert (sab);

  if (sab->len != sab->cap)
    {
      lalloc_r dims = lrealloc (
          sab->alloc,
          sab->dims,
          sab->len,
          sab->len,
          sizeof *sab->dims);

      if (dims.stat != AR_SUCCESS)
        {
          /*
           * Edge condition realloc shrink fail, treat as nomem
           */
          return error_causef (
              e, ERR_NOMEM,
              "Strict Array: Failed to shrink dims buffer");
        }

      ASSERT (dims.stat == AR_SUCCESS);

      sab->dims = dims.ret;
      sab->cap = dims.rlen;
    }

  return SUCCESS;
}

err_t
sab_build (sarray_t *dest, sarray_builder *sab, error *e)
{
  sarray_builder_assert (sab);
  ASSERT (dest);

  if (!sab->type)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Strict Array Builder: "
          "Can't build without a type");
    }
  if (sab->len == 0)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Strict Array Builder: "
          "Expect at least 1 dimension");
    }

  err_t_wrap (sab_clip (sab, e), e);

  dest->rank = sab->len;
  dest->dims = sab->dims;
  dest->t = sab->type;

  return SUCCESS;
}
