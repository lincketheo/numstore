#include "type/builders/sarray.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
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
sab_create (sarray_builder *dest, lalloc *alloc)
{
  ASSERT (dest);

  u32 *dims = lmalloc (alloc, 10 * sizeof *dims);
  if (!dims)
    {
      return ERR_NOMEM;
    }

  dest->type = NULL;
  dest->dims = dims;
  dest->len = 0;
  dest->cap = 10;
  dest->alloc = alloc;

  sarray_builder_assert (dest);

  return SUCCESS;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline err_t
sab_double_cap (sarray_builder *sab)
{
  sarray_builder_assert (sab);

  u32 cap = 2 * sab->cap;

  u32 *dims = lrealloc (sab->alloc, sab->dims, cap * sizeof *dims);
  if (!dims)
    {
      return ERR_NOMEM;
    }
  sab->cap = cap;
  sab->dims = dims;

  return SUCCESS;
}

err_t
sab_accept_dim (sarray_builder *sab, u32 dim)
{
  sarray_builder_assert (sab);

  err_t ret = SUCCESS;

  if (dim == 0)
    {
      return ERR_INVALID_ARGUMENT;
    }

  if (sab->len == sab->cap)
    {
      ret = sab_double_cap (sab);
      if (ret)
        {
          return ret;
        }
    }

  sab->dims[sab->len++] = dim;

  return SUCCESS;
}

err_t
sab_accept_type (sarray_builder *sab, type t)
{
  sarray_builder_assert (sab);

  if (sab->type)
    {
      return ERR_INVALID_ARGUMENT;
    }

  type *type = lmalloc (sab->alloc, sizeof *type);
  if (!type)
    {
      return ERR_NOMEM;
    }
  i_memcpy (type, &t, sizeof *type);

  sab->type = type;

  return SUCCESS;
}

static inline err_t
sab_clip (sarray_builder *sab)
{
  sarray_builder_assert (sab);

  if (sab->len != sab->cap)
    {
      u32 *dims = lrealloc (sab->alloc, sab->dims, sab->len * sizeof *dims);
      if (!dims)
        {
          return ERR_IO;
        }
    }

  return SUCCESS;
}

err_t
sab_build (sarray_t *dest, sarray_builder *sab)
{
  sarray_builder_assert (sab);
  ASSERT (dest);

  if (!sab->type)
    {
      return ERR_INVALID_ARGUMENT;
    }
  if (sab->len == 0)
    {
      return ERR_INVALID_ARGUMENT;
    }

  err_t ret = sab_clip (sab);
  if (ret)
    {
      return ret;
    }

  dest->rank = sab->len;
  dest->dims = sab->dims;
  dest->t = sab->type;

  return SUCCESS;
}
