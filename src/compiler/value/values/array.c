#include "compiler/value/values/array.h"
#include "compiler/value/value.h"
#include "core/errors/error.h"
#include "core/intf/stdlib.h"
#include "core/mm/lalloc.h"

bool
array_equal (const array *left, const array *right)
{
  if (left->len != right->len)
    {
      return false;
    }

  for (u32 i = 0; i < left->len; ++i)
    {
      if (!value_equal (&left->values[i], &right->values[i]))
        {
          return false;
        }
    }

  return true;
}

err_t
array_plus (array *dest, const array *right, lalloc *alloc, error *e)
{
  u32 len = dest->len + right->len;
  value *values = lmalloc (alloc, len, sizeof *values);
  if (values == NULL)
    {
      error_causef (e, ERR_NOMEM, "Failed to allocate values");
      return e->cause_code;
    }

  i_memcpy (values, dest->values, dest->len * sizeof *dest->values);
  i_memcpy (values + dest->len, right->values, right->len * sizeof *right->values);
  dest->len = len;

  return SUCCESS;
}
