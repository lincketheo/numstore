#include "compiler/value/value.h"
#include "compiler/value/values/array.h"
#include "compiler/value/values/object.h"
#include "core/dev/testing.h"
#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "core/utils/strings.h"
#include <complex.h>

const char *
value_t_tostr (value_t t)
{
  switch (t)
    {
      // Composite
    case VT_OBJECT:
      return "VT_OBJECT";
    case VT_ARRAY:
      return "VT_ARRAY";

      // Simple
    case VT_STRING:
      return "VT_STRING";
    case VT_NUMBER:
      return "VT_NUMBER";
    case VT_COMPLEX:
      return "VT_COMPLEX";
    case VT_TRUE:
      return "VT_TRUE";
    case VT_FALSE:
      return "VT_FALSE";
    }
}

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
    case VT_NUMBER:
      {
        return left->number == right->number;
      }
    case VT_COMPLEX:
      {
        return left->cplx == right->cplx;
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

#define CMP_OP(OP)                                                    \
  do                                                                  \
    {                                                                 \
      if (dest->type == VT_NUMBER && right->type == VT_NUMBER)        \
        {                                                             \
          if (dest->number OP right->number)                          \
            {                                                         \
              dest->type = VT_TRUE;                                   \
            }                                                         \
          else                                                        \
            {                                                         \
              dest->type = VT_FALSE;                                  \
            }                                                         \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      else if (dest->type == VT_COMPLEX && right->type == VT_NUMBER)  \
        {                                                             \
          if (dest->cplx OP (cf128) right->number)                    \
            {                                                         \
              dest->type = VT_TRUE;                                   \
            }                                                         \
          else                                                        \
            {                                                         \
              dest->type = VT_FALSE;                                  \
            }                                                         \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      else if (dest->type == VT_COMPLEX && right->type == VT_COMPLEX) \
        {                                                             \
          if (dest->cplx OP right->cplx)                              \
            {                                                         \
              dest->type = VT_TRUE;                                   \
            }                                                         \
          else                                                        \
            {                                                         \
              dest->type = VT_FALSE;                                  \
            }                                                         \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      else if (dest->type == VT_NUMBER && right->type == VT_COMPLEX)  \
        {                                                             \
          if ((cf128)dest->number OP right->cplx)                     \
            {                                                         \
              dest->type = VT_TRUE;                                   \
            }                                                         \
          else                                                        \
            {                                                         \
              dest->type = VT_FALSE;                                  \
            }                                                         \
          return SUCCESS;                                             \
        }                                                             \
    }                                                                 \
  while (0)

#define CMP_OP_CABS(OP)                                               \
  do                                                                  \
    {                                                                 \
      if (dest->type == VT_NUMBER && right->type == VT_NUMBER)        \
        {                                                             \
          if (dest->number OP right->number)                          \
            {                                                         \
              dest->type = VT_TRUE;                                   \
            }                                                         \
          else                                                        \
            {                                                         \
              dest->type = VT_FALSE;                                  \
            }                                                         \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      else if (dest->type == VT_COMPLEX && right->type == VT_NUMBER)  \
        {                                                             \
          if (cabs (dest->cplx) OP right->number)                     \
            {                                                         \
              dest->type = VT_TRUE;                                   \
            }                                                         \
          else                                                        \
            {                                                         \
              dest->type = VT_FALSE;                                  \
            }                                                         \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      else if (dest->type == VT_COMPLEX && right->type == VT_COMPLEX) \
        {                                                             \
          if (cabs (dest->cplx) OP cabs (right->cplx))                \
            {                                                         \
              dest->type = VT_TRUE;                                   \
            }                                                         \
          else                                                        \
            {                                                         \
              dest->type = VT_FALSE;                                  \
            }                                                         \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      else if (dest->type == VT_NUMBER && right->type == VT_COMPLEX)  \
        {                                                             \
          if (dest->number OP cabs (right->cplx))                     \
            {                                                         \
              dest->type = VT_TRUE;                                   \
            }                                                         \
          else                                                        \
            {                                                         \
              dest->type = VT_FALSE;                                  \
            }                                                         \
          return SUCCESS;                                             \
        }                                                             \
    }                                                                 \
  while (0)

#define NUMBER_COMPLEX_COERCE(OP)                                     \
  do                                                                  \
    {                                                                 \
      if (dest->type == VT_NUMBER && right->type == VT_NUMBER)        \
        {                                                             \
          dest->number = dest->number OP right->number;               \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      else if (dest->type == VT_COMPLEX && right->type == VT_NUMBER)  \
        {                                                             \
          dest->cplx = dest->cplx OP right->number;                   \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      else if (dest->type == VT_COMPLEX && right->type == VT_COMPLEX) \
        {                                                             \
          dest->cplx = dest->cplx OP right->cplx;                     \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      else if (dest->type == VT_NUMBER && right->type == VT_COMPLEX)  \
        {                                                             \
                                                                      \
          cf128 cplx = dest->number;                                  \
          cplx = cplx OP right->cplx;                                 \
          dest->cplx = cplx;                                          \
          dest->type = VT_COMPLEX;                                    \
                                                                      \
          return SUCCESS;                                             \
        }                                                             \
    }                                                                 \
  while (0)

#define ERR_UNSUPPORTED_BIN_OP(left, right, op)   \
  error_causef (                                  \
      e, ERR_SYNTAX,                              \
      "Unsupported operation type: %s for %s %s", \
      op, value_t_tostr (left->type), value_t_tostr (right->type))

#define ERR_UNSUPPORTED_UN_OP(dest, op)        \
  error_causef (                               \
      e, ERR_SYNTAX,                           \
      "Unsupported operation type: %s for %s", \
      op, value_t_tostr (dest->type))

static inline bool
value_truthy (const value *v)
{
  switch (v->type)
    {
    case VT_TRUE:
      {
        return true;
      }
    case VT_FALSE:
      {
        return false;
      }
    case VT_NUMBER:
      {
        return v->number != 0.0;
      }
    case VT_COMPLEX:
      {
        return cabs (v->cplx) != 0.0;
      }
    case VT_STRING:
      {
        return v->str.len != 0;
      }
    case VT_OBJECT:
      {
        return v->obj.len != 0;
      }
    case VT_ARRAY:
      {
        return v->arr.len != 0;
      }
    }
}

TEST (value_truthy)
{
  value v;

  v = (value){ .type = VT_TRUE };
  test_assert (value_truthy (&v));

  v = (value){ .type = VT_FALSE };
  test_assert (!value_truthy (&v));

  v = (value){ .type = VT_NUMBER, .number = 0.0 };
  test_assert (!value_truthy (&v));

  v = (value){ .type = VT_NUMBER, .number = 3.14 };
  test_assert (value_truthy (&v));

  v = (value){ .type = VT_COMPLEX, .cplx = 0 + 0 * I };
  test_assert (!value_truthy (&v));

  v = (value){ .type = VT_COMPLEX, .cplx = 3 + 4 * I }; // magnitude = 5
  test_assert (value_truthy (&v));

  v = (value){ .type = VT_STRING, .str = { .data = "", .len = 0 } };
  test_assert (!value_truthy (&v));

  v = (value){ .type = VT_STRING, .str = { .data = "hi", .len = 2 } };
  test_assert (value_truthy (&v));

  v = (value){ .type = VT_OBJECT, .obj = { .len = 0 } };
  test_assert (!value_truthy (&v));

  v = (value){ .type = VT_OBJECT, .obj = { .len = 2 } };
  test_assert (value_truthy (&v));

  v = (value){ .type = VT_ARRAY, .arr = { .len = 0 } };
  test_assert (!value_truthy (&v));

  v = (value){ .type = VT_ARRAY, .arr = { .len = 5 } };
  test_assert (value_truthy (&v));
}

err_t
value_plus_value (value *dest, const value *right, lalloc *alc, error *e)
{
  NUMBER_COMPLEX_COERCE (+);

  // ARRAY + ARRAY
  if (dest->type == VT_ARRAY && right->type == VT_ARRAY)
    {
      return array_plus (&dest->arr, &right->arr, alc, e);
    }

  // OBJECT + OBJECT
  else if (dest->type == VT_OBJECT && right->type == VT_OBJECT)
    {
      return object_plus (&dest->obj, &right->obj, alc, e);
    }

  // STRING + STRING
  else if (dest->type == VT_STRING && right->type == VT_STRING)
    {
      string next = string_plus (dest->str, right->str, alc, e);
      if (e->cause_code)
        {
          return e->cause_code;
        }
      dest->str = next;
      return SUCCESS;
    }

  return SUCCESS;
}
TEST (value_plus_value)
{
  u8 data[2048];
  lalloc alc = lalloc_create_from (data);
  error err = error_create (NULL);

  // Number + Number
  value a = { .type = VT_NUMBER, .number = 3.0 };
  value b = { .type = VT_NUMBER, .number = 4.5 };
  test_assert (value_plus_value (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == VT_NUMBER);
  test_assert (a.number == 7.5);

  // Complex + Complex
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (value_plus_value (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) == 4.0);
  test_assert (cimag (a.cplx) == 6.0);

  // Complex + Number
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (value){ .type = VT_NUMBER, .number = 5.0 };
  test_assert (value_plus_value (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) == 6.0);
  test_assert (cimag (a.cplx) == 2.0);

  // Number + Complex
  a = (value){ .type = VT_NUMBER, .number = 2.0 };
  b = (value){ .type = VT_COMPLEX, .cplx = 0.0 + 3.0 * I };
  test_assert (value_plus_value (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) == 2.0);
  test_assert (cimag (a.cplx) == 3.0);

  // String + String
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("foo") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("bar") };
  test_assert (value_plus_value (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.str.len == 6);
  test_assert (memcmp (a.str.data, "foobar", 6) == 0);

  // Array + Array
  // TODO

  // Object + Object
  // TODO
}

err_t
value_minus_value (value *dest, const value *right, error *e)
{
  NUMBER_COMPLEX_COERCE (-);

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "-");
}
TEST (value_minus_value)
{
  error err = error_create (NULL);

  // Number - Number
  value a = { .type = VT_NUMBER, .number = 5.0 };
  value b = { .type = VT_NUMBER, .number = 2.0 };
  test_assert (value_minus_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_NUMBER);
  test_assert (a.number == 3.0);

  // Complex - Complex
  a = (value){ .type = VT_COMPLEX, .cplx = 5.0 + 6.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  test_assert (value_minus_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) == 4.0);
  test_assert (cimag (a.cplx) == 4.0);

  // Number - Complex
  a = (value){ .type = VT_NUMBER, .number = 3.0 };
  b = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  test_assert (value_minus_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) == 2.0);
  test_assert (cimag (a.cplx) == -1.0);

  // Complex - Number
  a = (value){ .type = VT_COMPLEX, .cplx = 2.0 + 3.0 * I };
  b = (value){ .type = VT_NUMBER, .number = 1.0 };
  test_assert (value_minus_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) == 1.0);
  test_assert (cimag (a.cplx) == 3.0);

  // TODO: test array - array (should error)
  // TODO: test object - object (should error)
}

err_t
value_slash_value (value *dest, const value *right, error *e)
{
  NUMBER_COMPLEX_COERCE (/);

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "/");
}

TEST (value_slash_value)
{
  error err = error_create (NULL);

  // Number / Number
  value a = { .type = VT_NUMBER, .number = 10.0 };
  value b = { .type = VT_NUMBER, .number = 2.0 };
  test_assert (value_slash_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_NUMBER);
  test_assert (a.number == 5.0);

  // Complex / Complex
  a = (value){ .type = VT_COMPLEX, .cplx = 4.0 + 2.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  test_assert (value_slash_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) != 0.0 || cimag (a.cplx) != 0.0); // sanity check

  // Number / Complex
  a = (value){ .type = VT_NUMBER, .number = 4.0 };
  b = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  test_assert (value_slash_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);

  // Complex / Number
  a = (value){ .type = VT_COMPLEX, .cplx = 6.0 + 2.0 * I };
  b = (value){ .type = VT_NUMBER, .number = 2.0 };
  test_assert (value_slash_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) == 3.0);
  test_assert (cimag (a.cplx) == 1.0);

  // TODO: test array / array (should error)
  // TODO: test object / object (should error)
}

err_t
value_star_value (value *dest, const value *right, error *e)
{
  NUMBER_COMPLEX_COERCE (*);

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "*");
}

TEST (value_star_value)
{
  error err = error_create (NULL);

  // Number * Number
  value a = { .type = VT_NUMBER, .number = 3.0 };
  value b = { .type = VT_NUMBER, .number = 4.0 };
  test_assert (value_star_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_NUMBER);
  test_assert (a.number == 12.0);

  // Complex * Complex
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 2.0 + 3.0 * I };
  test_assert (value_star_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) != 0.0 || cimag (a.cplx) != 0.0); // sanity check

  // Number * Complex
  a = (value){ .type = VT_NUMBER, .number = 2.0 };
  b = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (value_star_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) == 6.0);
  test_assert (cimag (a.cplx) == 8.0);

  // Complex * Number
  a = (value){ .type = VT_COMPLEX, .cplx = 1.5 + 0.5 * I };
  b = (value){ .type = VT_NUMBER, .number = 2.0 };
  test_assert (value_star_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) == 3.0);
  test_assert (cimag (a.cplx) == 1.0);

  // TODO: test array * array (should error)
  // TODO: test object * object (should error)
}

static inline bool
value_t_is_bool (value_t t)
{
  return t == VT_TRUE || t == VT_FALSE;
}

err_t
value_equal_equal_value (value *dest, const value *right, error *e)
{
  CMP_OP (==);

  if (dest->type == VT_STRING && right->type == VT_STRING)
    {
      if (string_equal (dest->str, right->str))
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }
  else if (dest->type == VT_OBJECT && right->type == VT_OBJECT)
    {
      if (object_equal (&dest->obj, &right->obj))
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }
  else if (dest->type == VT_ARRAY && right->type == VT_ARRAY)
    {
      if (array_equal (&dest->arr, &right->arr))
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }
  else if (value_t_is_bool (dest->type))
    {
      if (dest->type == right->type)
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "==");
}

TEST (value_equal_equal_value)
{
  error err = error_create (NULL);

  // TRUE == TRUE
  value a = { .type = VT_TRUE };
  value b = { .type = VT_TRUE };
  test_assert (value_equal_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // TRUE == FALSE
  a = (value){ .type = VT_TRUE };
  b = (value){ .type = VT_FALSE };
  test_assert (value_equal_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // FALSE == FALSE
  a = (value){ .type = VT_FALSE };
  b = (value){ .type = VT_FALSE };
  test_assert (value_equal_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Number == Number (equal)
  a = (value){ .type = VT_NUMBER, .number = 3.0 };
  b = (value){ .type = VT_NUMBER, .number = 3.0 };
  test_assert (value_equal_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Number == Number (not equal)
  a = (value){ .type = VT_NUMBER, .number = 3.0 };
  b = (value){ .type = VT_NUMBER, .number = 2.0 };
  test_assert (value_equal_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // Complex == Complex (equal)
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  test_assert (value_equal_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Complex == Complex (not equal)
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 2.0 + 1.0 * I };
  test_assert (value_equal_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // String == String (equal)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("hello") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("hello") };
  test_assert (value_equal_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // String == String (not equal)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("hello") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("world") };
  test_assert (value_equal_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // TODO: test array == array (should error)
  // TODO: test object == object (should error)
}

err_t
value_bang_equal_value (value *dest, const value *right, error *e)
{
  CMP_OP (!=);

  if (dest->type == VT_STRING && right->type == VT_STRING)
    {
      if (!string_equal (dest->str, right->str))
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }
  else if (dest->type == VT_OBJECT && right->type == VT_OBJECT)
    {
      if (!object_equal (&dest->obj, &right->obj))
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }
  else if (dest->type == VT_ARRAY && right->type == VT_ARRAY)
    {
      if (!array_equal (&dest->arr, &right->arr))
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }
  else if (value_t_is_bool (dest->type))
    {
      if (dest->type != right->type)
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "!=");
}

TEST (value_bang_equal_value)
{
  error err = error_create (NULL);

  // TRUE != TRUE
  value a = { .type = VT_TRUE };
  value b = { .type = VT_TRUE };
  test_assert (value_bang_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // TRUE != FALSE
  a = (value){ .type = VT_TRUE };
  b = (value){ .type = VT_FALSE };
  test_assert (value_bang_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // FALSE != FALSE
  a = (value){ .type = VT_FALSE };
  b = (value){ .type = VT_FALSE };
  test_assert (value_bang_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // Number != Number (equal)
  a = (value){ .type = VT_NUMBER, .number = 5.0 };
  b = (value){ .type = VT_NUMBER, .number = 5.0 };
  test_assert (value_bang_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // Number != Number (not equal)
  a = (value){ .type = VT_NUMBER, .number = 3.0 };
  b = (value){ .type = VT_NUMBER, .number = 7.0 };
  test_assert (value_bang_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Complex != Complex (equal)
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  test_assert (value_bang_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // Complex != Complex (not equal)
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 2.0 + 2.0 * I };
  test_assert (value_bang_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // String != String (equal)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("same") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("same") };
  test_assert (value_bang_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // String != String (not equal)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("a") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("b") };
  test_assert (value_bang_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // TODO: test array != array (should error)
  // TODO: test object != object (should error)
}

err_t
value_greater_value (value *dest, const value *right, error *e)
{
  CMP_OP_CABS (>);

  if (dest->type == VT_STRING && right->type == VT_STRING)
    {
      if (string_greater_string (dest->str, right->str))
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, ">");
}

TEST (value_greater_value)
{
  error err = error_create (NULL);

  // TRUE > FALSE
  value a = { .type = VT_TRUE };
  value b = { .type = VT_FALSE };
  test_assert (value_greater_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // FALSE > TRUE
  a = (value){ .type = VT_FALSE };
  b = (value){ .type = VT_TRUE };
  test_assert (value_greater_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // Number > Number (true)
  a = (value){ .type = VT_NUMBER, .number = 10.0 };
  b = (value){ .type = VT_NUMBER, .number = 2.0 };
  test_assert (value_greater_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Number > Number (false)
  a = (value){ .type = VT_NUMBER, .number = 1.0 };
  b = (value){ .type = VT_NUMBER, .number = 4.0 };
  test_assert (value_greater_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // Complex > Complex (by magnitude: true)
  a = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I }; // mag = 5
  b = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 1.0 * I }; // mag ≈ 1.41
  test_assert (value_greater_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Complex > Complex (false)
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (value_greater_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // String > String (true)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("zebra") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("apple") };
  test_assert (value_greater_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // String > String (false)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("a") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("z") };
  test_assert (value_greater_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // TODO: test array > array (should error)
  // TODO: test object > object (should error)
}

err_t
value_greater_equal_value (value *dest, const value *right, error *e)
{
  CMP_OP_CABS (>=);

  if (dest->type == VT_STRING && right->type == VT_STRING)
    {
      if (string_greater_equal_string (dest->str, right->str))
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, ">=");
}

TEST (value_greater_equal_value)
{
  error err = error_create (NULL);

  // TRUE >= TRUE
  value a = { .type = VT_TRUE };
  value b = { .type = VT_TRUE };
  test_assert (value_greater_equal_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // TRUE >= FALSE
  a = (value){ .type = VT_TRUE };
  b = (value){ .type = VT_FALSE };
  test_assert (value_greater_equal_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // FALSE >= TRUE
  a = (value){ .type = VT_FALSE };
  b = (value){ .type = VT_TRUE };
  test_assert (value_greater_equal_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // Number >= Number (equal)
  a = (value){ .type = VT_NUMBER, .number = 5.0 };
  b = (value){ .type = VT_NUMBER, .number = 5.0 };
  test_assert (value_greater_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Number >= Number (true)
  a = (value){ .type = VT_NUMBER, .number = 10.0 };
  b = (value){ .type = VT_NUMBER, .number = 4.0 };
  test_assert (value_greater_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Number >= Number (false)
  a = (value){ .type = VT_NUMBER, .number = 2.0 };
  b = (value){ .type = VT_NUMBER, .number = 7.0 };
  test_assert (value_greater_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // Complex >= Complex (equal)
  a = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (value_greater_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Complex >= Complex (true)
  a = (value){ .type = VT_COMPLEX, .cplx = 5.0 + 0.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 0.0 * I };
  test_assert (value_greater_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Complex >= Complex (false)
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 4.0 + 3.0 * I };
  test_assert (value_greater_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // String >= String (equal)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("test") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("test") };
  test_assert (value_greater_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // String >= String (true)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("z") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("a") };
  test_assert (value_greater_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // String >= String (false)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("a") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("b") };
  test_assert (value_greater_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // TODO: test array >= array (should error)
  // TODO: test object >= object (should error)
}

err_t
value_less_value (value *dest, const value *right, error *e)
{
  CMP_OP_CABS (<);

  if (dest->type == VT_STRING && right->type == VT_STRING)
    {
      if (string_less_string (dest->str, right->str))
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "<");
}

TEST (value_less_value)
{
  error err = error_create (NULL);

  // TRUE < TRUE
  value a = { .type = VT_TRUE };
  value b = { .type = VT_TRUE };
  test_assert (value_less_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // TRUE < FALSE
  a = (value){ .type = VT_TRUE };
  b = (value){ .type = VT_FALSE };
  test_assert (value_less_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // FALSE < TRUE
  a = (value){ .type = VT_FALSE };
  b = (value){ .type = VT_TRUE };
  test_assert (value_less_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // Number < Number (true)
  a = (value){ .type = VT_NUMBER, .number = 3.0 };
  b = (value){ .type = VT_NUMBER, .number = 10.0 };
  test_assert (value_less_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Number < Number (false)
  a = (value){ .type = VT_NUMBER, .number = 10.0 };
  b = (value){ .type = VT_NUMBER, .number = 3.0 };
  test_assert (value_less_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // Number < Number (equal)
  a = (value){ .type = VT_NUMBER, .number = 3.0 };
  b = (value){ .type = VT_NUMBER, .number = 3.0 };
  test_assert (value_less_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // Complex < Complex (by magnitude: true)
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 1.0 * I }; // ≈ 1.41
  b = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I }; // = 5
  test_assert (value_less_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Complex < Complex (false)
  a = (value){ .type = VT_COMPLEX, .cplx = 5.0 + 0.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 2.0 + 0.0 * I };
  test_assert (value_less_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // Complex < Complex (equal)
  a = (value){ .type = VT_COMPLEX, .cplx = 2.0 + 2.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 2.0 + 2.0 * I };
  test_assert (value_less_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // String < String (true)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("apple") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("zebra") };
  test_assert (value_less_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // String < String (false)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("z") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("a") };
  test_assert (value_less_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // String < String (equal)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("abc") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("abc") };
  test_assert (value_less_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // TODO: test array < array (should error)
  // TODO: test object < object (should error)
}

err_t
value_less_equal_value (value *dest, const value *right, error *e)
{
  CMP_OP_CABS (<=);

  if (dest->type == VT_STRING && right->type == VT_STRING)
    {
      if (string_less_equal_string (dest->str, right->str))
        {
          dest->type = VT_TRUE;
        }
      else
        {
          dest->type = VT_FALSE;
        }
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "<=");
}

TEST (value_less_equal_value)
{
  error err = error_create (NULL);

  // TRUE <= TRUE
  value a = { .type = VT_TRUE };
  value b = { .type = VT_TRUE };
  test_assert (value_less_equal_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // TRUE <= FALSE
  a = (value){ .type = VT_TRUE };
  b = (value){ .type = VT_FALSE };
  test_assert (value_less_equal_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // FALSE <= TRUE
  a = (value){ .type = VT_FALSE };
  b = (value){ .type = VT_TRUE };
  test_assert (value_less_equal_value (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // Number <= Number (equal)
  a = (value){ .type = VT_NUMBER, .number = 5.0 };
  b = (value){ .type = VT_NUMBER, .number = 5.0 };
  test_assert (value_less_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Number <= Number (true)
  a = (value){ .type = VT_NUMBER, .number = 3.0 };
  b = (value){ .type = VT_NUMBER, .number = 7.0 };
  test_assert (value_less_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Number <= Number (false)
  a = (value){ .type = VT_NUMBER, .number = 9.0 };
  b = (value){ .type = VT_NUMBER, .number = 4.0 };
  test_assert (value_less_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // Complex <= Complex (equal)
  a = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I }; // mag = 5
  b = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I }; // mag = 5
  test_assert (value_less_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Complex <= Complex (true)
  a = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 1.0 * I }; // mag ≈ 1.41
  b = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I }; // mag = 5
  test_assert (value_less_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // Complex <= Complex (false)
  a = (value){ .type = VT_COMPLEX, .cplx = 5.0 + 0.0 * I };
  b = (value){ .type = VT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  test_assert (value_less_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // String <= String (equal)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("abc") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("abc") };
  test_assert (value_less_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // String <= String (true)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("abc") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("bcd") };
  test_assert (value_less_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_TRUE);

  // String <= String (false)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("xyz") };
  b = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("abc") };
  test_assert (value_less_equal_value (&a, &b, &err) == SUCCESS);
  test_assert (a.type == VT_FALSE);

  // TODO: test array <= array (should error)
  // TODO: test object <= object (should error)
}

err_t
value_minus (value *dest, error *e)
{
  switch (dest->type)
    {
    case VT_COMPLEX:
      {
        dest->cplx = -dest->cplx;
        return SUCCESS;
      }
    case VT_NUMBER:
      {
        dest->number = -dest->number;
        return SUCCESS;
      }
    default:
      {
        break;
      }
    }

  return ERR_UNSUPPORTED_UN_OP (dest, "-");
}

TEST (value_minus)
{
  error err = error_create (NULL);

  // -Number
  value a = { .type = VT_NUMBER, .number = 5.0 };
  test_assert (value_minus (&a, &err) == SUCCESS);
  test_assert (a.type == VT_NUMBER);
  test_assert (a.number == -5.0);

  // -(-Number)
  a = (value){ .type = VT_NUMBER, .number = -3.0 };
  test_assert (value_minus (&a, &err) == SUCCESS);
  test_assert (a.type == VT_NUMBER);
  test_assert (a.number == 3.0);

  // -Complex
  a = (value){ .type = VT_COMPLEX, .cplx = 2.0 + 3.0 * I };
  test_assert (value_minus (&a, &err) == SUCCESS);
  test_assert (a.type == VT_COMPLEX);
  test_assert (creal (a.cplx) == -2.0);
  test_assert (cimag (a.cplx) == -3.0);

  // -True (should fail)
  a = (value){ .type = VT_TRUE };
  test_assert (value_minus (&a, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // -False (should fail)
  a = (value){ .type = VT_FALSE };
  test_assert (value_minus (&a, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // -String (should fail)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("bad") };
  test_assert (value_minus (&a, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // -Array (should fail)
  a = (value){ .type = VT_ARRAY, .arr = { .len = 0 } };
  test_assert (value_minus (&a, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // -Object (should fail)
  a = (value){ .type = VT_OBJECT, .obj = { .len = 0 } };
  test_assert (value_minus (&a, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;
}

void
value_bang (value *dest)
{
  if (!value_truthy (dest))
    {
      dest->type = VT_TRUE;
    }
  else
    {
      dest->type = VT_FALSE;
    }
}

TEST (value_bang)
{
  // Number (non-zero → false)
  value a = { .type = VT_NUMBER, .number = 42.0 };
  value_bang (&a);
  test_assert (a.type == VT_FALSE);

  // Number (zero → true)
  a = (value){ .type = VT_NUMBER, .number = 0.0 };
  value_bang (&a);
  test_assert (a.type == VT_TRUE);

  // Complex (non-zero → false)
  a = (value){ .type = VT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  value_bang (&a);
  test_assert (a.type == VT_FALSE);

  // Complex (zero → true)
  a = (value){ .type = VT_COMPLEX, .cplx = 0.0 + 0.0 * I };
  value_bang (&a);
  test_assert (a.type == VT_TRUE);

  // True → false
  a = (value){ .type = VT_TRUE };
  value_bang (&a);
  test_assert (a.type == VT_FALSE);

  // False → true
  a = (value){ .type = VT_FALSE };
  value_bang (&a);
  test_assert (a.type == VT_TRUE);

  // String (non-empty → false)
  a = (value){ .type = VT_STRING, .str = unsafe_cstrfrom ("hello") };
  value_bang (&a);
  test_assert (a.type == VT_FALSE);

  // String (empty → true)
  a = (value){ .type = VT_STRING, .str = { .data = "", .len = 0 } };
  value_bang (&a);
  test_assert (a.type == VT_TRUE);

  // Array (non-empty → false)
  a = (value){ .type = VT_ARRAY, .arr = { .len = 1 } };
  value_bang (&a);
  test_assert (a.type == VT_FALSE);

  // Array (empty → true)
  a = (value){ .type = VT_ARRAY, .arr = { .len = 0 } };
  value_bang (&a);
  test_assert (a.type == VT_TRUE);

  // Object (non-empty → false)
  a = (value){ .type = VT_OBJECT, .obj = { .len = 2 } };
  value_bang (&a);
  test_assert (a.type == VT_FALSE);

  // Object (empty → true)
  a = (value){ .type = VT_OBJECT, .obj = { .len = 0 } };
  value_bang (&a);
  test_assert (a.type == VT_TRUE);
}

void
i_log_value (value *v)
{
  switch (v->type)
    {
    case VT_OBJECT:
      {
        i_log_info ("====== OBJECT: \n");
        for (u32 i = 0; i < v->obj.len; ++i)
          {
            i_log_info ("Key: %.*s. Value: \n", v->obj.keys[i].len, v->obj.keys[i].data);
            i_log_value (&v->obj.values[i]);
          }
        i_log_info ("====== DONE \n");
        return;
      }
    case VT_ARRAY:
      {
        i_log_info ("====== ARRAY: \n");
        for (u32 i = 0; i < v->arr.len; ++i)
          {
            i_log_value (&v->arr.values[i]);
          }
        i_log_info ("====== DONE \n");
        return;
      }
    case VT_STRING:
      {
        i_log_info ("%.*s\n", v->str.len, v->str.data);
        return;
      }
    case VT_NUMBER:
      {
        i_log_info ("%Lf\n", v->number);
        return;
      }
    case VT_COMPLEX:
      {
        i_log_info ("%f %f\n", creal (v->cplx), cimag (v->cplx));
        return;
      }
    case VT_TRUE:
      {
        i_log_info ("TRUE\n");
        return;
      }
    case VT_FALSE:
      {
        i_log_info ("FALSE\n");
        return;
      }
    }
}
