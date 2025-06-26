#include "compiler/value/values/array.h"
#include "compiler/value/value.h"

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
