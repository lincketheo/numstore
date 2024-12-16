//
// Created by theo on 12/15/24.
//

#ifndef TESTING_H
#define TESTING_H

extern "C" {
////////////////////////////////////////////////////////////
// Test framework definitions (enabled when NTEST is not defined)
#ifndef NTEST

#include "numstore/errors.hpp"  // fail
#include "numstore/logging.hpp" // ns_log
#include "numstore/macros.hpp"  // Colors
#include "numstore/types.hpp"   // size_t

#define MAX_TESTS_LEN 2048 // Maximum number of test functions allowed

// Type definition for a test function
typedef void (*test_func)();

extern test_func tests[MAX_TESTS_LEN]; // Array to store registered test functions
extern size_t test_count; // Number of registered test functions

// Macro to define a test case
// Usage:
// TEST(foo) {
//    TEST_EQUAL(1, 1, "%d");
// }
#define TEST(name)                              \
  void test_##name ();                          \
  void wrapper_test_##name ()                   \
  {                                             \
    ns_log (INFO, "Running Test %s\n", #name);                  \
    test_##name ();                             \
    ns_log (INFO, BOLD_GREEN "PASSED\n" RESET);    \
  }                                             \
  CONSTRUCTOR PRIVATE void register_##name ()   \
  {                                             \
    if (test_count >= MAX_TESTS_LEN)            \
      {                                         \
        fatal_error ("Increase MAX_TESTS_LEN"); \
      }                                         \
    tests[test_count++] = wrapper_test_##name;  \
  }                                             \
  void test_##name ()

void test_assert_str_equal(const char *str1, const char *str2);

void test_assert_f32_equal(float left, float right, float tol);

// Macro to assert that two values are equal
#define test_assert_equal(left, right, fmt)                                               \
  if ((left) != (right))                                                                  \
    {                                                                                     \
      ns_log (ERROR, BOLD_RED "FAILED: " RESET RED "%s != %s -- " fmt " != " fmt "\n" RESET, \
           #left, #right, left, right);                                                   \
      fatal_error (LOCATION_FSTRING "\n", LOCATION_FARGS);                                \
    }

#define test_assert_not_equal(left, right, fmt)                                               \
  if ((left) == (right))                                                                  \
    {                                                                                     \
      ns_log (ERROR, BOLD_RED "FAILED: " RESET RED "%s == %s -- " fmt " == " fmt "\n" RESET, \
           #left, #right, left, right);                                                   \
      fatal_error (LOCATION_FSTRING "\n", LOCATION_FARGS);                                \
    }

#else

// Define a no-op for TEST when testing is disabled
#define TEST(name, body)

#endif // End of NTEST
////////////////////////////////////////////////////////////
}


#endif //TESTING_H
