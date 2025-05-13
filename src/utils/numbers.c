#include "utils/numbers.h"

#include "dev/assert.h"   // ASSERT
#include "dev/testing.h"  // TEST
#include "errors/error.h" // err_t
#include "intf/stdlib.h"  // i_unsafe_strlen
#include "intf/types.h"   // i32 / f32
#include "utils/bounds.h" // SAFE_...
#include "utils/macros.h" // is_num

err_t
parse_i32_expect (i32 *dest, const string data, error *e)
{
  ASSERT (data.data);
  ASSERT (data.len > 0);
  ASSERT (dest);

  u32 i = 0;
  bool neg = false;

  if (data.data[i] == '+' || data.data[i] == '-')
    {
      neg = (data.data[i] == '-');
      i++;
      ASSERT (i < data.len); // We expect string to be valid
    }

  i32 acc = 0;

  for (; i < data.len; i++)
    {
      char c = data.data[i];
      ASSERT (is_num (c));

      int32_t digit = c - '0';

      if (!SAFE_MUL_I32 (&acc, acc, 10))
        {
          goto failed;
        }

      if (!SAFE_SUB_I32 (&acc, acc, digit))
        {
          goto failed;
        }
    }

  if (!neg)
    {
      if (acc == I32_MIN)
        {
          goto failed;
        }
      acc = -acc;
    }

  *dest = acc;
  return SUCCESS;

failed:
  return error_causef (
      e, ERR_ARITH,
      "Parse I32: Arithmetic Exception");
}

TEST (parse_i32_expect)
{
  i32 out;
  error e = error_create (NULL);

  const string s1 = (string){
    .data = "1234",
    .len = i_unsafe_strlen ("1234"),
  };
  test_assert_int_equal (parse_i32_expect (&out, s1, &e), SUCCESS);
  test_assert_int_equal (out, 1234);

  const string s2 = (string){
    .data = "-56",
    .len = i_unsafe_strlen ("-56"),
  };
  test_assert_int_equal (parse_i32_expect (&out, s2, &e), SUCCESS);
  test_assert_int_equal (out, -56);

  char *big = "999999999999999999999999999999999999999999";
  const string s3 = (string){
    .data = big,
    .len = i_unsafe_strlen (big),
  };
  test_assert_int_equal (parse_i32_expect (&out, s3, &e), ERR_ARITH);
}

err_t
parse_f32_expect (f32 *dest, const string src, error *e)
{
  ASSERT (src.data);
  ASSERT (dest);
  ASSERT (src.len > 0);

  u32 len = src.len;
  char *s = src.data;
  u32 i = 0;
  bool neg = false;

  if (s[i] == '+' || s[i] == '-')
    {
      neg = (s[i] == '-');
      i++;
      ASSERT (i < len);
    }

  // Integer part
  f32 acc = 0.0f;
  bool saw_digit = false;
  while (i < len && s[i] >= '0' && s[i] <= '9')
    {
      f32 d = (f32)(s[i] - '0');
      if (!SAFE_MUL_F32 (&acc, acc, 10.0f))
        {
          goto failed;
        }
      if (!SAFE_ADD_F32 (&acc, acc, d))
        {
          goto failed;
        }
      i++;
      saw_digit = true;
    }

  // Fractional part
  if (i < len && s[i] == '.')
    {
      i++;
      ASSERT (i < len); // cannot end with '.'
      f32 frac = 0.0f, scale = 1.0f;
      while (i < len && s[i] >= '0' && s[i] <= '9')
        {
          f32 d = (f32)(s[i] - '0');
          if (!SAFE_MUL_F32 (&frac, frac, 10.0f))
            {
              goto failed;
            }
          if (!SAFE_ADD_F32 (&frac, frac, d))
            {
              goto failed;
            }
          if (!SAFE_MUL_F32 (&scale, scale, 10.0f))
            {
              goto failed;
            }
          i++;
          saw_digit = true;
        }
      f32 tmp;
      if (!SAFE_DIV_F32 (&tmp, frac, scale))
        {
          goto failed;
        }
      if (!SAFE_ADD_F32 (&acc, acc, tmp))
        {
          goto failed;
        }
    }

  ASSERT (saw_digit);

  // Exponent part
  if (i < len && (s[i] == 'e' || s[i] == 'E'))
    {
      i++;
      ASSERT (i < len); // must have exponent digits
      bool exp_neg = false;
      if (s[i] == '+' || s[i] == '-')
        {
          exp_neg = (s[i] == '-');
          i++;
          ASSERT (i < len);
        }
      u32 exp = 0;
      bool saw_exp = false;
      while (i < len && s[i] >= '0' && s[i] <= '9')
        {
          u32 d = (u32)(s[i] - '0');
          ASSERT (exp <= (UINT32_MAX - d) / 10);
          exp = exp * 10 + d;
          i++;
          saw_exp = true;
        }
      ASSERT (saw_exp);

      // Apply exponent
      for (u32 k = 0; k < exp; k++)
        {
          if (exp_neg)
            {
              if (!SAFE_DIV_F32 (&acc, acc, 10.0f))
                {
                  goto failed;
                }
            }
          else
            {
              if (!SAFE_MUL_F32 (&acc, acc, 10.0f))
                {
                  goto failed;
                }
            }
        }
    }

  ASSERT (i == len); // no extra characters

  if (neg)
    acc = -acc;
  *dest = acc;
  return SUCCESS;

failed:
  return error_causef (
      e, ERR_ARITH,
      "Parse F32: Arithmetic Exception");
}

#define EPSILON 1e-6f

TEST (parse_f32_expect)
{
  f32 out;
  error e = error_create (NULL);

  const string s1 = {
    .data = "3.14",
    .len = i_unsafe_strlen ("3.14"),
  };
  test_assert_int_equal (parse_f32_expect (&out, s1, &e), SUCCESS);
  test_assert_int_equal (fabsf (out - 3.14f) < EPSILON, true);

  const string s2 = {
    .data = "-0.5",
    .len = i_unsafe_strlen ("-0.5"),
  };
  test_assert_int_equal (parse_f32_expect (&out, s2, &e), SUCCESS);
  test_assert_int_equal (fabsf (out + 0.5f) < EPSILON, true);

  const string s3 = {
    .data = "1.23e3",
    .len = i_unsafe_strlen ("1.23e3"),
  };
  test_assert_int_equal (parse_f32_expect (&out, s3, &e), SUCCESS);
  test_assert_int_equal (fabsf (out - 1230.0f) < EPSILON, true);

  const string s4 = {
    .data = ".25",
    .len = i_unsafe_strlen (".25"),
  };
  test_assert_int_equal (parse_f32_expect (&out, s4, &e), SUCCESS);
  test_assert_int_equal (fabsf (out - 0.25f) < EPSILON, true);

  const string s5 = {
    .data = "1e40",
    .len = i_unsafe_strlen ("1e40"),
  };
  test_assert_int_equal (parse_f32_expect (&out, s5, &e), ERR_ARITH);
}
