#pragma once

#ifndef NTEST

#include "ns_errors.h"
#include "ns_macros.h"
#include "ns_types.h"

#include <stdio.h>

#define MAX_TESTS_LEN 2048

typedef void (*test_func) ();
extern test_func tests[MAX_TESTS_LEN];
extern ns_size test_count;

#define TEST_REPORT()                                                         \
  fprintf (stderr, "%s(%s:%d)\n", __FILE__, __func__, __LINE__);

#define TEST(name)                                                            \
  void test_##name ();                                                        \
  void wrapper_test_##name ()                                                 \
  {                                                                           \
    fprintf (stdout, "%s: ", #name);                                          \
    test_##name ();                                                           \
    fprintf (stdout, BOLD_GREEN "PASSED\n" RESET);                            \
  }                                                                           \
  __attribute__ ((constructor)) static void register_##name ()                \
  {                                                                           \
    if (test_count >= MAX_TESTS_LEN)                                          \
      {                                                                       \
        fprintf (stderr, "Increase MAX_TESTS_LEN");                           \
        fail ();                                                              \
      }                                                                       \
    tests[test_count++] = wrapper_test_##name;                                        \
  }                                                                           \
  void test_##name ()

#define TEST_ASSERT_STR_EQUAL(str1, str2)                                     \
  if (strcmp (str1, str2) != 0)                                               \
    {                                                                         \
      fprintf (stderr,                                                        \
               BOLD_RED "FAILED: " RESET RED "\"%s\" != \"%s\" " RESET, str1, \
               str2);                                                         \
      TEST_REPORT ();                                                         \
      ASSERT (0);                                                             \
    }

#define TEST_EQUAL(left, right, fmt)                                          \
  if (left != right)                                                          \
    {                                                                         \
      fprintf (stderr,                                                        \
               BOLD_RED "FAILED: " RESET RED "%s != %s -- " fmt " != " fmt    \
                        "\n" RESET,                                           \
               #left, #right, left, right);                                   \
      TEST_REPORT ();                                                         \
      ASSERT (0);                                                             \
    }

#else

#define TEST(name, body)

#endif
