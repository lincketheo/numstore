#include "types.hpp"
#include "testing.hpp"

#include <cstring>

usize
srange_copy (u8 *dest, usize dnelem, const u8 *src, usize snelem, srange range,
             usize size)
{
  assert (dest);
  assert (src);
  assert (size > 0);

  usize di = 0, si = range.start;
  for (; si < range.end && si < snelem && di < dnelem; si += range.span, di++)
    {
      std::memcpy (dest + di * size, src + si * size, size);
    }
  return di;
}

TEST (srange_copy)
{
  u32 dest[10];
  u32 src[20] = { 1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                  11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
  u32 expect[10] = { 3, 5, 7, 9, 11, 13, 15, 17, 19 };

  srange r = {
    .start = 2,
    .end = USIZE_MAX,
    .span = 2,
  };

  usize copied = srange_copy ((u8 *)dest, sizeof (dest), (u8 *)src, 20, r,
                              sizeof (u32));

  test_assert_equal (copied, 9lu, "%zu");
  test_assert_equal (std::memcmp (dest, expect, 9 * sizeof (u32)), 0, "%d");
  test_assert_equal(1, 0, "%d");
}
