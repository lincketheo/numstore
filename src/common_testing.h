#pragma once

#ifndef NTEST

#include "common_assert.h"  // ASSERT
#include "common_logging.h" // fprintf_...
#include "common_macros.h"  // colors
#include "common_string.h"  // safe_strnlen

// Main testing data
#define MAX_TESTS_LEN 2048
#define MAX_TEST_STREQUAL 2048
typedef void (*test_func) ();
extern test_func tests[MAX_TESTS_LEN];
extern size_t test_count;

// Register a test function
#define REGISTER_TEST(func)                                                   \
  void run_test_##func ()                                                     \
  {                                                                           \
    fprintf (stdout, "TEST %s: ", #func);                                     \
    if (func ())                                                              \
      {                                                                       \
        fprintf_err (BOLD_RED "FAILED\n" RESET);                              \
      }                                                                       \
    else                                                                      \
      {                                                                       \
        fprintf_log (BOLD_GREEN "PASSED \n" RESET);                           \
      }                                                                       \
  }                                                                           \
  __attribute__ ((constructor)) static void register_##func ()                \
  {                                                                           \
    ASSERT (test_count < MAX_TESTS_LEN && "Increase MAX_TESTS_LEN");          \
    tests[test_count++] = run_test_##func;                                    \
  }

// Test comparison operators
#define TEST_ASSERT_STR_EQUAL(str1, str2)                                     \
  if (!safe_strnequal (str1, str2, MAX_TEST_STREQUAL))                        \
    {                                                                         \
      fprintf_err (BOLD_RED "FAILED: " RESET RED "\"%s\" != \"%s\" " RESET,   \
                   str1, str2);                                               \
      return -1;                                                              \
    }
#define TEST_EQUAL(left, right, fmt)                                          \
  if (left != right)                                                          \
    {                                                                         \
      fprintf_err (BOLD_RED "FAILED: " RESET RED "%s != %s -- " fmt           \
                            " != " fmt "\n" RESET,                            \
                   #left, #right, left, right);                               \
      return -1;                                                              \
    }

#else

#define TEST(name, body)

#endif
