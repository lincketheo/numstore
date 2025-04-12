#pragma once

#include "common/types.h"
#include "dev/logging.h"

typedef void (*test_func)(void);
extern u64 ntests;
extern test_func tests[2048];
extern int test_ret;

#define TEST(name)                                               \
  static void test_##name(void);                                 \
  static void wrapper_test_##name(void)                          \
  {                                                              \
    LOG_INFO("========================= TEST CASE: %s", #name);  \
    int prev = test_ret;                                         \
    test_ret = 0;                                                \
    test_##name();                                               \
    if (test_ret) {                                              \
      LOG_FAILURE("%s", #name);                                  \
    } else {                                                     \
      LOG_SUCCESS("%s", #name);                                  \
      test_ret = prev;                                           \
    }                                                            \
  }                                                              \
  __attribute__((constructor)) static void register_##name(void) \
  {                                                              \
    assert(ntests < 2048);                                       \
    tests[ntests++] = wrapper_test_##name;                       \
  }                                                              \
  static void test_##name(void)

#define test_assert_equal(left, right, fmt)                                   \
  if ((left) != (right)) {                                                    \
    LOG_FAILURE("%s != %s (" fmt " != " fmt ")", #left, #right, left, right); \
    test_ret = -1;                                                            \
  } else {                                                                    \
    LOG_SUCCESS("%s == %s (" fmt " == " fmt ")", #left, #right, left, right); \
  }

#define test_assert_int_equal(left, right) test_assert_equal(left, right, "%" PRId32)
