#pragma once

#include "dev/assert.h"
#include "os/types.h"

typedef void (*test_func) (void);
extern u64 ntests;
extern test_func tests[2048];
extern int test_ret;

#define TEST(name)                                                 \
  static void test_##name (void);                                  \
  static void wrapper_test_##name (void)                           \
  {                                                                \
    i_log_info ("========================= TEST CASE: %s", #name); \
    int prev = test_ret;                                           \
    test_ret = 0;                                                  \
    test_##name ();                                                \
    if (test_ret)                                                  \
      {                                                            \
        i_log_failure ("%s", #name);                               \
      }                                                            \
    else                                                           \
      {                                                            \
        i_log_success ("%s", #name);                               \
        test_ret = prev;                                           \
      }                                                            \
  }                                                                \
  __attribute__ ((constructor)) static void register_##name (void) \
  {                                                                \
    ASSERT (ntests < 2048);                                        \
    tests[ntests++] = wrapper_test_##name;                         \
  }                                                                \
  static void test_##name (void)

#define test_assert_equal(left, right, fmt)                                            \
  do                                                                                   \
    {                                                                                  \
      if ((left) != (right))                                                           \
        {                                                                              \
          i_log_failure ("%s != %s (" fmt " != " fmt ")", #left, #right, left, right); \
          test_ret = -1;                                                               \
          return;                                                                      \
        }                                                                              \
    }                                                                                  \
  while (0)

#define test_assert_int_equal(left, right) test_assert_equal (left, right, "%" PRId32)

#define test_fail_if(expr)                                                 \
  do                                                                       \
    {                                                                      \
      if (expr)                                                            \
        {                                                                  \
          i_log_failure ("Failed test due to invalid value: %s\n", #expr); \
          test_ret = -1;                                                   \
          return;                                                          \
        }                                                                  \
    }                                                                      \
  while (0)

#define test_fail_if_null(expr)                                              \
  do                                                                         \
    {                                                                        \
      if (expr == NULL)                                                      \
        {                                                                    \
          i_log_failure ("Failed test due to unexpected NULL: %s\n", #expr); \
          test_ret = -1;                                                     \
          return;                                                            \
        }                                                                    \
    }                                                                        \
  while (0)
