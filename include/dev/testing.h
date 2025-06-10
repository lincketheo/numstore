#pragma once

#include "dev/assert.h"  // ASSERT
#include "intf/stdlib.h" // i_unsafe_strlen
#include "intf/types.h"  // u32

typedef void (*test_func) (void);
typedef struct
{
  test_func test;
  const char *test_name;
  u32 nlen;
} test;

extern u64 ntests;       // Number of tests
extern test tests[2048]; // The actual tests
extern int test_ret;     // The return value of all tests

#define TEST(name)                                                   \
  static void test_##name (void);                                    \
  static void wrapper_test_##name (void)                             \
  {                                                                  \
    i_log_info ("========================= TEST CASE: %s\n", #name); \
    int prev = test_ret;                                             \
    test_ret = 0;                                                    \
    test_##name ();                                                  \
    if (test_ret)                                                    \
      {                                                              \
        i_log_failure ("%s at %s:%d (%s)\n",                         \
                       #name, __FILE__, __LINE__, __func__);         \
      }                                                              \
    else                                                             \
      {                                                              \
        i_log_passed ("%s\n", #name);                                \
        test_ret = prev;                                             \
      }                                                              \
  }                                                                  \
  __attribute__ ((constructor)) static void register_##name (void)   \
  {                                                                  \
    ASSERT (ntests < 2048);                                          \
    test next = {                                                    \
      .test = wrapper_test_##name,                                   \
      .test_name = #name,                                            \
      .nlen = i_unsafe_strlen (#name),                               \
    };                                                               \
    tests[ntests++] = next;                                          \
  }                                                                  \
  static void test_##name (void)

#define test_assert_equal(left, right)                 \
  do                                                   \
    {                                                  \
      if ((left) != (right))                           \
        {                                              \
          i_log_failure ("%s != %s\n", #left, #right); \
          test_ret = -1;                               \
          return;                                      \
        }                                              \
    }                                                  \
  while (0)

#define test_assert_int_equal(left, right)                          \
  do                                                                \
    {                                                               \
      int _left = left;                                             \
      int _right = right;                                           \
      if ((_left) != (_right))                                      \
        {                                                           \
          i_log_failure ("%d %s != %s\n", __LINE__, #left, #right); \
          i_log_failure ("(%d) != (%d)\n", _left, _right);          \
          test_ret = -1;                                            \
          return;                                                   \
        }                                                           \
    }                                                               \
  while (0)

#define test_assert_ptr_equal(left, right) test_assert_equal ((void *)left, (void *)right)

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
