#include "types.h"
#include "testing.h"

#include <stdint.h>
#include <string.h>

u64 dtype_sizeof(const dtype type) {
  switch (type) {
  case U8:
  case I8:
    return 1;
  case U16:
  case I16:
    return 2;
  case U32:
  case I32:
  case F32:
    return 4;
  case U64:
  case I64:
  case F64:
    return 8;
  case F128:
    return 16;
  case CF64:
  case CI16:
  case CU16:
    return 8;
  case CF128:
  case CI32:
  case CU32:
    return 16;
  case CF256:
  case CI64:
  case CU64:
    return 32;
  case CI128:
  case CU128:
    return 64;
  }
  return 0;
}

u64 srange_copy(u8 *dest, u64 dnelem, const u8 *src, u64 snelem, srange range,
                u64 size) {
  assert(dest);
  assert(src);
  assert(size > 0);

  if (range.span == 0 || range.start >= snelem || range.start >= range.end) {
    return 0;
  }

  u64 di = 0, si = range.start;
  for (; si < range.end && si < snelem && di < dnelem; si += range.span, di++) {
    memcpy(dest + di * size, src + si * size, size);
  }
  return di;
}

TEST(srange_copy) {
  u32 dest[10];
  u32 src[20] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  u32 expect[10] = {3, 5, 7, 9, 11, 13, 15, 17, 19};

  srange r = {.start = 2, .end = UINT64_MAX, .span = 2};
  u64 copied = srange_copy((u8 *)dest, 10, (u8 *)src, 20, r, sizeof(u32));
  test_assert_equal(copied, (u64)9, "%" PRIu64);
  test_assert_equal(memcmp(dest, expect, 9 * sizeof(u32)), 0, "%d");

  srange r1 = {.start = 5, .end = 5, .span = 1};
  copied = srange_copy((u8 *)dest, 10, (u8 *)src, 20, r1, sizeof(u32));
  test_assert_equal(copied, (u64)0, "%" PRIu64);

  srange r2 = {.start = 0, .end = 2, .span = 5};
  copied = srange_copy((u8 *)dest, 10, (u8 *)src, 20, r2, sizeof(u32));
  test_assert_equal(copied, (u64)1, "%" PRIu64);

  srange r3 = {.start = 25, .end = 30, .span = 1};
  copied = srange_copy((u8 *)dest, 10, (u8 *)src, 20, r3, sizeof(u32));
  test_assert_equal(copied, (u64)0, "%" PRIu64);

  srange r4 = {.start = 0, .end = 10, .span = 0};
  copied = srange_copy((u8 *)dest, 10, (u8 *)src, 20, r4, sizeof(u32));
  test_assert_equal(copied, (u64)0, "%" PRIu64);

  srange r5 = {.start = 0, .end = 30, .span = 3};
  copied = srange_copy((u8 *)dest, 10, (u8 *)src, 20, r5, sizeof(u32));
  test_assert_equal(copied, (u64)7, "%" PRIu64);

  u32 small_dest[5];
  srange r6 = {.start = 0, .end = 10, .span = 1};
  copied = srange_copy((u8 *)small_dest, 5, (u8 *)src, 20, r6, sizeof(u32));
  test_assert_equal(copied, (u64)5, "%" PRIu64);

  u32 large_dest[30];
  srange r8 = {.start = 0, .end = 20, .span = 1};
  copied = srange_copy((u8 *)large_dest, 30, (u8 *)src, 20, r8, sizeof(u32));
  test_assert_equal(copied, (u64)20, "%" PRIu64);
}
