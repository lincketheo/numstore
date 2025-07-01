#include "compiler/value/values/object.h"
#include "compiler/value/value.h"
#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "core/intf/stdlib.h"
#include "core/utils/strings.h"

i32
object_t_snprintf (char *str, u32 size, const object *st)
{
  (void)str;
  (void)size;
  (void)st;
  return -1;
}

bool
object_equal (const object *left, const object *right)
{
  if (left->len != right->len)
    {
      return false;
    }

  for (u32 i = 0; i < left->len; ++i)
    {
      if (!string_equal (left->keys[i], right->keys[i]))
        {
          return false;
        }
      if (!value_equal (&left->values[i], &right->values[i]))
        {
          return false;
        }
    }

  return true;
}

err_t
object_plus (object *dest, const object *right, lalloc *alloc, error *e)
{
  // Check for duplicate keys
  const string *duplicate = strings_are_disjoint (dest->keys, dest->len, right->keys, right->len);
  if (duplicate != NULL)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Cannot merge two objects with duplicate keys: %.*s",
          duplicate->len, duplicate->data);
    }

  u32 len = dest->len + right->len;

  string *keys = lmalloc (alloc, len, sizeof *keys);
  value *values = lmalloc (alloc, len, sizeof *values);
  if (values == NULL || keys == NULL)
    {
      error_causef (e, ERR_NOMEM, "Failed to allocate dest object");
      return e->cause_code;
    }

  // Copy over values
  i_memcpy (values, dest->values, dest->len * sizeof *dest->values);
  i_memcpy (values + dest->len, right->values, right->len * sizeof *right->values);

  // Copy over keys
  i_memcpy (keys, dest->keys, dest->len * sizeof *dest->keys);
  i_memcpy (keys + dest->len, right->keys, right->len * sizeof *right->keys);

  dest->len = len;

  return SUCCESS;
}
