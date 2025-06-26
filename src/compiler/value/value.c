#include "compiler/value/value.h"
#include "core/ds/strings.h"

bool
value_equal (const value *left, const value *right)
{
  if (left->type != right->type)
    {
      return false;
    }

  switch (left->type)
    {
      // Composite
    case VT_OBJECT:
      {
        return object_equal (&left->obj, &right->obj);
      }
    case VT_ARRAY:
      {
        return array_equal (&left->arr, &right->arr);
      }

      // Simple
    case VT_STRING:
      {
        return string_equal (left->str, right->str);
      }
    case VT_IDENT:
      {
        return string_equal (left->ident, right->ident);
      }
    case VT_NUMBER:
      {
        return left->number == right->number;
      }
    case VT_COMPLEX:
      {
        return left->complex == right->complex;
      }
    case VT_TRUE:
      {
        return true;
      }
    case VT_FALSE:
      {
        return true;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}
