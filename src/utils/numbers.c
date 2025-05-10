#include "utils/numbers.h"

#include "dev/assert.h"   // ASSERT
#include "dev/testing.h"  // TEST
#include "intf/stdlib.h"  // i_unsafe_strlen
#include "utils/macros.h" // is_num

i32
parse_i32_expect (const string data)
{
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
      ret = ret * 10 + _next;
    }

  if (negative)
    {
      return -1 * (i32)ret;
    }
  else
    {
      return (i32)ret;
    }
}

TEST (parse_i32_expect)
{
  i32 out;

  const string s1 = (string){
    .data = "1234",
    .len = i_unsafe_strlen ("1234"),
  };
  out = parse_i32_expect (s1);
  test_assert_int_equal (out, 1234);

  const string s2 = (string){
    .data = "-56",
    .len = i_unsafe_strlen ("-56"),
  };
  out = parse_i32_expect (s2);
  test_assert_int_equal (out, -56);

  // TODO - handle int overflow
}

f32
parse_f32_expect (const string src)
{
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
      return -ret;
    }
  else
    {
      return ret;
    }
}

TEST (parse_f32_expect)
{
  f32 out;

  const string b1 = (string){
    .data = "12.34",
    .len = i_unsafe_strlen ("12.34"),
  };
  out = parse_f32_expect (b1);
  test_assert_equal ((int)(out * 100), 1234, "%d");

  const string b2 = (string){
    .data = "-0.5",
    .len = i_unsafe_strlen ("-0.5"),
  };
  out = parse_f32_expect (b2);
  test_assert_equal ((int)(out * 10), -5, "%d");

  const string b3 = (string){
    .data = "100",
    .len = i_unsafe_strlen ("100"),
  };
  out = parse_f32_expect (b3);
  test_assert_equal ((int)out, 100, "%d");

  const string b4 = (string){
    .data = "12.34",
    .len = i_unsafe_strlen ("12.34"),
  };
  out = parse_f32_expect (b4);
  test_assert_equal ((int)(out * 1000), 12340, "%d");
}
