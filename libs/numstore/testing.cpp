//
// Created by theo on 12/15/24.
//

#include "numstore/testing.hpp"
#include "numstore/errors.hpp"
#include "numstore/macros.hpp"

extern "C" {
#include <math.h>
#include <string.h>

#ifndef NTEST
test_func tests[MAX_TESTS_LEN];
size_t test_count = 0;

void
test_assert_str_equal(const char *str1, const char *str2) {
    if (strcmp(str1, str2) != 0) {
        ns_log(ERROR, BOLD_RED "FAILED: " RESET RED "\"%s\" != \"%s\"\n" RESET, str1, str2);
        fatal_error(LOCATION_FSTRING "\n", LOCATION_FARGS);
    }
}

void
test_assert_f32_equal(const float left, const float right, const float tol) {
    if (fabsf(left - right) > tol) {
        ns_log(ERROR, BOLD_RED "FAILED: " RESET RED "\"%f\" != \"%f\"\n" RESET, left, right);
        fatal_error(LOCATION_FSTRING "\n", LOCATION_FARGS);
    }
}
}
#endif
