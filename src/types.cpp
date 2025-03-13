#include "types.hpp"
#include "testing.hpp"

#include <cstring>

usize
srange_copy (u8 *dest, usize dnelem, const u8 *src, usize snelem, srange range, usize size)
{
  assert (dest);
  assert (src);
  assert (size > 0);

  if (range.span == 0 || range.start >= snelem || range.start >= range.end) {
    return 0;
  }

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
  u32 src[20] = { 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
  u32 expect[10] = { 3, 5, 7, 9, 11, 13, 15, 17, 19 };

  srange r = { .start = 2, .end = USIZE_MAX, .span = 2 };
  usize copied = srange_copy ((u8 *)dest, sizeof (dest), (u8 *)src, 20, r, sizeof (u32));
  test_assert_equal (copied, 9lu, "%zu");
  test_assert_equal (std::memcmp (dest, expect, 9 * sizeof (u32)), 0, "%d");

  srange r1 = { .start = 5, .end = 5, .span = 1 };
  copied = srange_copy((u8 *)dest, sizeof(dest), (u8 *)src, 20, r1, sizeof(u32));
  test_assert_equal(copied, 0lu, "%zu");

  srange r2 = { .start = 0, .end = 2, .span = 5 };
  copied = srange_copy((u8 *)dest, sizeof(dest), (u8 *)src, 20, r2, sizeof(u32));
  test_assert_equal(copied, 1lu, "%zu");

  srange r3 = { .start = 25, .end = 30, .span = 1 };
  copied = srange_copy((u8 *)dest, sizeof(dest), (u8 *)src, 20, r3, sizeof(u32));
  test_assert_equal(copied, 0lu, "%zu");

  srange r4 = { .start = 0, .end = 10, .span = 0 };
  copied = srange_copy((u8 *)dest, sizeof(dest), (u8 *)src, 20, r4, sizeof(u32));
  test_assert_equal(copied, 0lu, "%zu");

  srange r5 = { .start = 0, .end = 30, .span = 3 };
  copied = srange_copy((u8 *)dest, sizeof(dest), (u8 *)src, 20, r5, sizeof(u32));
  test_assert_equal(copied, 7lu, "%zu");

  u32 small_dest[5];
  srange r6 = { .start = 0, .end = 10, .span = 1 };
  copied = srange_copy((u8 *)small_dest, sizeof(small_dest), (u8 *)src, 20, r6, sizeof(u32));
  test_assert_equal(copied, 5lu, "%zu");

  u32 empty_src[0];
  srange r7 = { .start = 0, .end = 10, .span = 1 };
  copied = srange_copy((u8 *)dest, sizeof(dest), (u8 *)empty_src, 0, r7, sizeof(u32));
  test_assert_equal(copied, 0lu, "%zu");

  u32 large_dest[30];
  srange r8 = { .start = 0, .end = 20, .span = 1 };
  copied = srange_copy((u8 *)large_dest, sizeof(large_dest), (u8 *)src, 20, r8, sizeof(u32));
  test_assert_equal(copied, 20lu, "%zu");
}

