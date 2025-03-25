#pragma once

#include "macros.h"
#include "types.h"

#include <assert.h>
#include <stdio.h>

typedef void (*test_func)();
extern usize ntests;
extern test_func tests[2048];
extern int test_ret;

// Macro to define a test
#define TEST(name)                                                    \
  static void test_##name();                                          \
  static void wrapper_test_##name()                                   \
  {                                                                   \
    fprintf(stderr,                                                   \
        BOLD_WHITE "========================= TEST CASE: %s\n" RESET, \
        #name);                                                       \
    int prev = test_ret;                                              \
    test_ret = 0;                                                     \
    test_##name();                                                    \
    if (test_ret) {                                                   \
      fprintf(stderr, BOLD_RED "FAILED: %s\n", #name);                \
    } else {                                                          \
      fprintf(stderr, BOLD_GREEN "SUCCESS: %s\n", #name);             \
      test_ret = prev;                                                \
    }                                                                 \
  }                                                                   \
  __attribute__((constructor)) static void register_##name()          \
  {                                                                   \
    assert(ntests < 2048);                                            \
    tests[ntests++] = wrapper_test_##name;                            \
  }                                                                   \
  static void test_##name()

// Macro for asserting equality
#define test_assert_equal(left, right, fmt)                               \
  if ((left) != (right)) {                                                \
    fprintf(stderr, RED "%s != %s (" fmt " != " fmt ")\n" RESET, #left,   \
        #right, left, right);                                             \
    test_ret = -1;                                                        \
  } else {                                                                \
    fprintf(stderr, GREEN "%s == %s (" fmt " == " fmt ")\n" RESET, #left, \
        #right, left, right);                                             \
  }
