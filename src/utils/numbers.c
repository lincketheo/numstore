#include "dev/assert.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/stdlib.h"
#include "sds.h"
#include "types.h"
#include "utils/bounds.h"
#include "utils/macros.h"

err_t
smlst_dbl_fctr (u32 *cap, u32 len, u32 nbytes)
{
  ASSERT (cap);
  ASSERT (*cap > 0);

  if (!can_add_u32 (len, nbytes))
    {
      return ERR_ARITH;
    }
  u32 total = len + nbytes;

  if (!can_add_u32 (total, *cap - 1))
    {
      return ERR_ARITH;
    }

  ASSERT (can_div_u32 (total + *cap - 1, *cap));
  u32 target = (total + *cap - 1) / *cap;

  if (target <= 1)
    {
      return SUCCESS;
    }

  target--;
  target |= target >> 1;
  target |= target >> 2;
  target |= target >> 4;
  target |= target >> 8;
  target |= target >> 16;
  target++;

  *cap = target * *cap;
  return SUCCESS;
}

TEST (smlst_dbl_fctr)
{
  {
    u32 cap = 64;
    u32 len = 10;
    u32 nbytes = 20;
    err_t err = smlst_dbl_fctr (&cap, len, nbytes);

    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (cap, 64);
  }
  {
    u32 cap = 32;
    u32 len = 20;
    u32 nbytes = 10;
    err_t err = smlst_dbl_fctr (&cap, len, nbytes);
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (cap, 32);
  }

  {
    u32 cap = 64;
    u32 len = 60;
    u32 nbytes = 10;
    err_t err = smlst_dbl_fctr (&cap, len, nbytes);
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (cap, 128);
  }

  {
    u32 cap = 32;
    u32 len = U32_MAX - 10;
    u32 nbytes = 20;
    err_t err = smlst_dbl_fctr (&cap, len, nbytes);
    test_assert_int_equal (err, ERR_ARITH);
  }

  {
    u32 cap = 4;
    u32 len = U32_MAX - 2;
    u32 nbytes = 1;
    err_t err = smlst_dbl_fctr (&cap, len, nbytes);
    test_assert_int_equal (err, ERR_ARITH);
  }
}

err_t
parse_i32_expect (i32 *dest, const string data)
{
  ASSERT (dest);
  string_assert (&data);
  ASSERT (data.len > 0);

  int negative = 0;
  u32 ret = 0;

  u32 i = 0;
  u8 next = data.data[0];

  if (next == '-')
    {
      negative = 1;
      i++;
    }
  else if (next == '+')
    {
      i++;
    }

  ASSERT (i < data.len); // Should have at least 1 digit

  for (; i < data.len; ++i)
    {
      next = data.data[i];
      ASSERT (is_num (next));
      u32 _next = next - '0';
      if (!can_mul_u32 (ret, 10) || !can_add_u32 (ret * 10, _next))
        {
          return ERR_ARITH;
        }
      ret = ret * 10 + _next;
    }

  if (negative)
    {
      *dest = -1 * (i32)ret;
    }
  else
    {
      *dest = (i32)ret;
    }
  return SUCCESS;
}

TEST (parse_i32_expect)
{
  i32 out;

  const string s1 = (string){ .data = "1234X", .len = i_unsafe_strlen ("1234X") };
  test_assert_int_equal (parse_i32_expect (&out, s1), SUCCESS);
  test_assert_int_equal (out, 1234);

  const string s2 = (string){ .data = "-56Y", .len = i_unsafe_strlen ("-56Y") };
  test_assert_int_equal (parse_i32_expect (&out, s2), SUCCESS);
  test_assert_int_equal (out, -56);

  const string s3 = (string){ .data = "9999999999Z", .len = i_unsafe_strlen ("9999999999Z") };
  test_assert_int_equal (parse_i32_expect (&out, s3), ERR_OVERFLOW);
}

int
parse_f32_expect (f32 *dest, const string src)
{
  ASSERT (dest);
  string_assert (&src);
  ASSERT (src.len > 0);

  int negative = 0;
  f32 ret = 0.0f;
  u32 frac = 0;
  f32 frac_div = 1.0f;
  int seen_dot = 0;

  u32 i = 0;
  u8 next = src.data[0];

  if (next == '-')
    {
      negative = 1;
      i++;
    }
  else if (next == '+')
    {
      i++;
    }

  ASSERT (i < src.len); // Should have at least 1 digit

  for (; i < src.len; ++i)
    {
      next = src.data[i];

      if (next == '.')
        {
          ASSERT (!seen_dot);
          seen_dot = 1;
          continue;
        }

      ASSERT (is_num (next));
      u32 _next = next - '0';

      if (!seen_dot)
        {
          if (!can_mul_f32 (ret, 10.0f) || !can_add_f32 (ret * 10.f, _next))
            {
              return ERR_ARITH;
            }

          ret = ret * 10.0f + (f32)_next;
        }
      else
        {
          frac = frac * 10 + _next;
          frac_div *= 10.0f;
        }
    }

  ret += (f32)frac / frac_div;

  if (negative)
    {
      *dest = -ret;
    }
  else
    {
      *dest = ret;
    }

  return SUCCESS;
}

TEST (cbuffer_parse_front_f32)
{
  f32 out;

  const string b1 = (string){ .data = "12.34", .len = i_unsafe_strlen ("12.34") };
  test_assert_int_equal (parse_f32_expect (&out, b1), SUCCESS);
  test_assert_equal ((int)(out * 100), 1234, "%d");

  const string b2 = (string){ .data = "-0.5", .len = i_unsafe_strlen ("-0.5") };
  test_assert_int_equal (parse_f32_expect (&out, b2), SUCCESS);
  test_assert_equal ((int)(out * 10), -5, "%d");

  const string b3 = (string){ .data = "100", .len = i_unsafe_strlen ("100") };
  test_assert_int_equal (parse_f32_expect (&out, b3), SUCCESS);
  test_assert_equal ((int)out, 100, "%d");

  const string b4 = (string){ .data = "12.34", .len = i_unsafe_strlen ("12.34") };
  test_assert_int_equal (parse_f32_expect (&out, b4), SUCCESS);
  test_assert_equal ((int)(out * 1000), 345, "%d");

  const string b5 = (string){ .data = "99999999999999999999999", .len = i_unsafe_strlen ("99999999999999999999999") };
  test_assert_int_equal (parse_f32_expect (&out, b5), ERR_ARITH);
  test_assert_equal ((int)(out * 1000), 345, "%d");
}
