#include "compiler/value/values/object.h"
#include "compiler/value/value.h"
#include "core/ds/strings.h"

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
