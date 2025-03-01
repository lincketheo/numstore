#pragma once

#include "types.hpp"

#include <cassert>

typedef void (*test_func) ();
extern usize ntests;
extern test_func tests[2048];

#define TEST(name)                                                            \
  static void test_##name ();                                                 \
  static void wrapper_test_##name () { test_##name (); }                      \
  __attribute__ ((constructor)) static void register_##name ()                \
  {                                                                           \
    assert (ntests < 2048);                                                   \
    tests[ntests++] = wrapper_test_##name;                                    \
  }                                                                           \
  static void test_##name ()

#define test_assert_equal(left, right, fmt)                                   \
  if (left != right)                                                          \
    {                                                                         \
      fprintf (stderr, "FAILED: %s != %s -- " fmt " != " fmt "\n", #left, #right,  \
               left, right);                                                  \
    }
