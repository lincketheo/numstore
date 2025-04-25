#include "dev/assert.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/stdlib.h"
#include "sds.h"
#include "types.h"
#include "utils/bounds.h"
#include "utils/macros.h"

u32
smlst_dbl_fctr (u32 cap, u32 len, u32 nbytes)
{
  ASSERT (cap > 0);

  ASSERT (can_add_u32 (len, nbytes));
  u32 total = len + nbytes;
  ASSERT (can_add_u32 (total, cap - 1));
  ASSERT (can_div_u32 (total + cap - 1, cap));
  u32 target = (total + cap - 1) / cap;

  if (target <= 1)
    {
      return cap;
    }

  target--;
  target |= target >> 1;
  target |= target >> 2;
  target |= target >> 4;
  target |= target >> 8;
  target |= target >> 16;
  target++;

  return cap * target;
}

TEST (smlst_dbl_fctr)
{
  test_assert_int_equal (smlst_dbl_fctr (64, 10, 20), 64);
  test_assert_int_equal (smlst_dbl_fctr (32, 20, 10), 32);
  test_assert_int_equal (smlst_dbl_fctr (64, 60, 10), 128);
}

err_t
parse_i32_expect (i32 *dest, const string data)
{
  ASSERT (dest);
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

  const string s1 = (string){ .data = "1234", .len = i_unsafe_strlen ("1234") };
  test_assert_int_equal (parse_i32_expect (&out, s1), SUCCESS);
  test_assert_int_equal (out, 1234);

  const string s2 = (string){ .data = "-56", .len = i_unsafe_strlen ("-56") };
  test_assert_int_equal (parse_i32_expect (&out, s2), SUCCESS);
  test_assert_int_equal (out, -56);

  const string s3 = (string){ .data = "9999999999", .len = i_unsafe_strlen ("9999999999") };
  test_assert_int_equal (parse_i32_expect (&out, s3), ERR_ARITH);
}

int
parse_f32_expect (f32 *dest, const string src)
{
  ASSERT (dest);
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

TEST (parse_f32_expect)
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
  test_assert_equal ((int)(out * 1000), 12340, "%d");
}
