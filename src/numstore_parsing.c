#include "numstore_parsing.h"

#include "common_testing.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
try_parse_f32 (float *dest, const char *str)
{
  char *end;
  float ret = strtof (str, &end);
  if (*end == '\0')
    {
      *dest = ret;
      return 0;
    }
  return -1;
}

#ifndef NTEST
int
test_try_parse_f32 ()
{
  float res;
  int ret = try_parse_f32 (&res, "1.123");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 1.123f, "%f");

  ret = try_parse_f32 (&res, "1.");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 1.0f, "%f");

  ret = try_parse_f32 (&res, ".12");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, .12f, "%f");

  ret = try_parse_f32 (&res, "-.12");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, -.12f, "%f");

  ret = try_parse_f32 (&res, "1e10");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 1e10f, "%f");

  ret = try_parse_f32 (&res, "a");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_f32 (&res, "a1.123");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_f32 (&res, "1.123a");
  TEST_EQUAL (ret, -1, "%d");
  return 0;
}
REGISTER_TEST (test_try_parse_f32);
#endif

int
try_parse_f64 (double *dest, const char *str)
{
  char *endptr;
  errno = 0;

  double value = strtod (str, &endptr);

  if ((errno == ERANGE && (value == HUGE_VAL || value == -HUGE_VAL))
      || *endptr != '\0' || str == endptr)
    {
      return -1;
    }

  *dest = value;
  return 0;
}

#ifndef NTEST
int
test_try_parse_f64 ()
{
  double res;
  int ret = try_parse_f64 (&res, "1.0");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 1.0, "%f");

  ret = try_parse_f64 (&res, "-1.5");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, -1.5, "%f");

  ret = try_parse_f64 (&res, "0.0");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 0.0, "%f");

  ret = try_parse_f64 (&res, "1e3");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 1000.0, "%f");

  ret = try_parse_f64 (&res, "1e-3");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 0.001, "%f");

  ret = try_parse_f64 (&res, "a");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_f64 (&res, "123abc");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_f64 (&res, "1.8e+308"); // DBL_MAX
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_f64 (&res, "-1.8e+308"); // -DBL_MAX
  TEST_EQUAL (ret, -1, "%d");
  return 0;
}
REGISTER_TEST (test_try_parse_f64);
#endif

int
try_parse_i32 (int *dest, const char *str)
{
  char *end;
  float ret = strtof (str, &end);
  if (*end == '\0')
    {
      *dest = ret;
      return 0;
    }
  return -1;
}

#ifndef NTEST
int
test_try_parse_i32 ()
{
  int res;
  int ret = try_parse_i32 (&res, "1");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 1, "%d");

  ret = try_parse_i32 (&res, "-1");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, -1, "%d");

  ret = try_parse_i32 (&res, "0");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 0, "%d");

  ret = try_parse_i32 (&res, "-1.123");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, -1, "%d");

  ret = try_parse_i32 (&res, "1e2");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 100, "%d");

  ret = try_parse_i32 (&res, "a");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_i32 (&res, "a1");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_i32 (&res, "1a");
  TEST_EQUAL (ret, -1, "%d");

  return 0;
}
REGISTER_TEST (test_try_parse_i32);
#endif

int
try_parse_u32 (uint32_t *dest, const char *str)
{
  char *end;
  unsigned long ret = strtoul (str, &end, 10);
  if (*end == '\0' && ret <= UINT32_MAX)
    {
      *dest = (uint32_t)ret;
      return 0;
    }
  return -1;
}

#ifndef NTEST
int
test_try_parse_u32 ()
{
  uint32_t res;

  int ret = try_parse_u32 (&res, "1");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 1, "%u");

  ret = try_parse_u32 (&res, "0");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 0, "%u");

  ret = try_parse_u32 (&res, "4294967295");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 4294967295u, "%u");

  ret = try_parse_u32 (&res, "4294967296");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u32 (&res, "-1");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u32 (&res, "1.5");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u32 (&res, "1e3");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u32 (&res, "a123");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u32 (&res, "123a");
  TEST_EQUAL (ret, -1, "%d");

  return 0;
}
REGISTER_TEST (test_try_parse_u32);
#endif

int
try_parse_u64 (uint64_t *dest, const char *str)
{
  char *endptr;
  errno = 0;

  if (*str == '-')
    {
      return -1;
    }

  unsigned long long value = strtoull (str, &endptr, 10);

  if (errno == ERANGE || value > UINT64_MAX || *endptr != '\0'
      || str == endptr)
    {
      return -1;
    }

  *dest = (uint64_t)value;
  return 0;
}

#ifndef NTEST
int
test_try_parse_u64 ()
{
  uint64_t res;
  int ret = try_parse_u64 (&res, "1");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 1UL, "%" PRIu64);

  ret = try_parse_u64 (&res, "123456789");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 123456789UL, "%" PRIu64);

  ret = try_parse_u64 (&res, "0");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 0UL, "%" PRIu64);

  ret = try_parse_u64 (&res, "18446744073709551615");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 18446744073709551615UL, "%" PRIu64);

  ret = try_parse_u64 (&res, "-1");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u64 (&res, "1.23");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u64 (&res, "1e5");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u64 (&res, "a");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u64 (&res, "123abc");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u64 (&res, "18446744073709551616");
  TEST_EQUAL (ret, -1, "%d");

  return 0;
}
REGISTER_TEST (test_try_parse_u64);
#endif

int
try_parse_u64_neg (uint64_t *dest, int *isneg, const char *str)
{
  char *endptr;
  errno = 0;

  if (*str == '-')
    {
      *isneg = 1;
      str++;
    }
  else
    {
      *isneg = 0;
    }

  unsigned long long value = strtoull (str, &endptr, 10);

  if (errno == ERANGE || value > UINT64_MAX || *endptr != '\0'
      || str == endptr)
    {
      return -1;
    }

  *dest = (uint64_t)value;
  return 0;
}

#ifndef NTEST
int
test_try_parse_u64_neg ()
{
  uint64_t res;
  int isneg;
  int ret;

  ret = try_parse_u64_neg (&res, &isneg, "123");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 123LU, "%" PRIu64);
  TEST_EQUAL (isneg, 0, "%d");

  ret = try_parse_u64_neg (&res, &isneg, "-123");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 123LU, "%" PRIu64);
  TEST_EQUAL (isneg, 1, "%d");

  ret = try_parse_u64_neg (&res, &isneg, "18446744073709551615");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 18446744073709551615UL, "%" PRIu64);
  TEST_EQUAL (isneg, 0, "%d");

  ret = try_parse_u64_neg (&res, &isneg, "-18446744073709551615");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (res, 18446744073709551615UL, "%" PRIu64);
  TEST_EQUAL (isneg, 1, "%d");

  ret = try_parse_u64_neg (&res, &isneg, "18446744073709551616");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u64_neg (&res, &isneg, "abc");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_u64_neg (&res, &isneg, "-abc");
  TEST_EQUAL (ret, -1, "%d");

  return 0;
}
REGISTER_TEST (test_try_parse_u64_neg);
#endif

int
try_parse_cf128 (double complex *dest, const char *str)
{
  double real, imag;
  char sign;

  int matched = sscanf (str, "%lf %c i%lf", &real, &sign, &imag);

  if (matched == 3 && (sign == '+' || sign == '-'))
    {
      if (sign == '-')
        {
          imag = -imag;
        }
      *dest = real + imag * I;
      return 0;
    }

  return -1;
}

#ifndef NTEST
int
test_try_parse_cf128 ()
{
  double complex res;
  int ret;

  ret = try_parse_cf128 (&res, "3.0+i4.0");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (creal (res), 3.0, "%f");
  TEST_EQUAL (cimag (res), 4.0, "%f");

  ret = try_parse_cf128 (&res, "2.5-i1.5");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (creal (res), 2.5, "%f");
  TEST_EQUAL (cimag (res), -1.5, "%f");

  ret = try_parse_cf128 (&res, "5.0");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_cf128 (&res, "i2.0");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_cf128 (&res, "3.0+4.0i");
  TEST_EQUAL (ret, -1, "%d");

  ret = try_parse_cf128 (&res, " 3.0 + i4.0 ");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (creal (res), 3.0, "%f");
  TEST_EQUAL (cimag (res), 4.0, "%f");

  ret = try_parse_cf128 (&res, "-3.0+i2.0");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (creal (res), -3.0, "%f");
  TEST_EQUAL (cimag (res), 2.0, "%f");

  ret = try_parse_cf128 (&res, "3.0-i2.0");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (creal (res), 3.0, "%f");
  TEST_EQUAL (cimag (res), -2.0, "%f");

  ret = try_parse_cf128 (&res, "3.0+i0.0");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (creal (res), 3.0, "%f");
  TEST_EQUAL (cimag (res), 0.0, "%f");

  ret = try_parse_cf128 (&res, "0.0+i0.0");
  TEST_EQUAL (ret, 0, "%d");
  TEST_EQUAL (creal (res), 0.0, "%f");
  TEST_EQUAL (cimag (res), 0.0, "%f");
  return 0;
}
REGISTER_TEST (test_try_parse_cf128);
#endif
