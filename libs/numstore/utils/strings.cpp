#include "numstore/utils/strings.hpp"
#include "numstore/datastruct.hpp"
#include "numstore/errors.hpp"
#include "numstore/testing.hpp"

extern "C" {
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>

int construct_path(const char *base, const char *suffix, char *dest,
                   size_t dlen) {
  ASSERT(base);
  ASSERT(suffix);
  ASSERT(dest);
  ASSERT(dlen > 0);

  if (!base || !suffix || !dest || dlen == 0) {
    return -1;
  }

  size_t blen = strnlen(base, dlen - 1);
  size_t slen = strlen(suffix);

  if (blen + slen >= dlen) {
    return -1;
  }

  strncpy(dest, base, blen);
  dest[blen] = '\0';

  strncat(dest, suffix, dlen - blen - 1);

  return 0;
}

int try_parse_f32(float *dest, const char *str) {
  char *end;
  float ret = strtof(str, &end);
  if (*end == '\0') {
    *dest = ret;
    return 0;
  }
  return -1;
}

TEST(try_parse_f32) {
  float res;
  int ret = try_parse_f32(&res, "1.123");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 1.123f, "%f");

  ret = try_parse_f32(&res, "1.");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 1.0f, "%f");

  ret = try_parse_f32(&res, ".12");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, .12f, "%f");

  ret = try_parse_f32(&res, "-.12");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, -.12f, "%f");

  ret = try_parse_f32(&res, "1e10");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 1e10f, "%f");

  ret = try_parse_f32(&res, "a");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_f32(&res, "a1.123");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_f32(&res, "1.123a");
  test_assert_equal(ret, -1, "%d");
}

int try_parse_f64(double *dest, const char *str) {
  char *endptr;
  errno = 0;

  double value = strtod(str, &endptr);

  if ((errno == ERANGE && (value == HUGE_VAL || value == -HUGE_VAL)) ||
      *endptr != '\0' || str == endptr) {
    return -1;
  }

  *dest = value;
  return 0;
}

TEST(try_parse_f64) {
  double res;
  int ret = try_parse_f64(&res, "1.0");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 1.0, "%f");

  ret = try_parse_f64(&res, "-1.5");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, -1.5, "%f");

  ret = try_parse_f64(&res, "0.0");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 0.0, "%f");

  ret = try_parse_f64(&res, "1e3");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 1000.0, "%f");

  ret = try_parse_f64(&res, "1e-3");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 0.001, "%f");

  ret = try_parse_f64(&res, "a");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_f64(&res, "123abc");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_f64(&res, "1.8e+308"); // DBL_MAX
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_f64(&res, "-1.8e+308"); // -DBL_MAX
  test_assert_equal(ret, -1, "%d");
}

int try_parse_i32(int *dest, const char *str) {
  char *end;
  float ret = strtof(str, &end);
  if (*end == '\0') {
    *dest = ret;
    return 0;
  }
  return -1;
}

TEST(try_parse_i32) {
  int res;
  int ret = try_parse_i32(&res, "1");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 1, "%d");

  ret = try_parse_i32(&res, "-1");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, -1, "%d");

  ret = try_parse_i32(&res, "0");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 0, "%d");

  ret = try_parse_i32(&res, "-1.123");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, -1, "%d");

  ret = try_parse_i32(&res, "1e2");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 100, "%d");

  ret = try_parse_i32(&res, "a");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_i32(&res, "a1");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_i32(&res, "1a");
  test_assert_equal(ret, -1, "%d");
}

int try_parse_u32(uint32_t *dest, const char *str) {
  char *end;
  unsigned long ret = strtoul(str, &end, 10);
  if (*end == '\0' && ret <= UINT32_MAX) {
    *dest = (uint32_t)ret;
    return 0;
  }
  return -1;
}
TEST(try_parse_u32) {
  uint32_t res;

  int ret = try_parse_u32(&res, "1");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 1, "%u");

  ret = try_parse_u32(&res, "0");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 0, "%u");

  ret = try_parse_u32(&res, "4294967295");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 4294967295u, "%u");

  ret = try_parse_u32(&res, "4294967296");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u32(&res, "-1");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u32(&res, "1.5");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u32(&res, "1e3");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u32(&res, "a123");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u32(&res, "123a");
  test_assert_equal(ret, -1, "%d");
}

int try_parse_u64(uint64_t *dest, const char *str) {
  char *endptr;
  errno = 0;

  if (*str == '-') {
    return -1;
  }

  unsigned long long value = strtoull(str, &endptr, 10);

  if (errno == ERANGE || value > UINT64_MAX || *endptr != '\0' ||
      str == endptr) {
    return -1;
  }

  *dest = (uint64_t)value;
  return 0;
}

TEST(try_parse_u64) {
  uint64_t res;
  int ret = try_parse_u64(&res, "1");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 1UL, "%" PRIu64);

  ret = try_parse_u64(&res, "123456789");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 123456789UL, "%" PRIu64);

  ret = try_parse_u64(&res, "0");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 0UL, "%" PRIu64);

  ret = try_parse_u64(&res, "18446744073709551615");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 18446744073709551615UL, "%" PRIu64);

  ret = try_parse_u64(&res, "-1");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u64(&res, "1.23");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u64(&res, "1e5");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u64(&res, "a");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u64(&res, "123abc");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u64(&res, "18446744073709551616");
  test_assert_equal(ret, -1, "%d");
}

int try_parse_u64_neg(uint64_t *dest, int *isneg, const char *str) {
  char *endptr;
  errno = 0;

  if (*str == '-') {
    *isneg = 1;
    str++;
  } else {
    *isneg = 0;
  }

  unsigned long long value = strtoull(str, &endptr, 10);

  if (errno == ERANGE || value > UINT64_MAX || *endptr != '\0' ||
      str == endptr) {
    return -1;
  }

  *dest = (uint64_t)value;
  return 0;
}

TEST(try_parse_u64_neg) {
  uint64_t res;
  int isneg;
  int ret;

  ret = try_parse_u64_neg(&res, &isneg, "123");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 123LU, "%" PRIu64);
  test_assert_equal(isneg, 0, "%d");

  ret = try_parse_u64_neg(&res, &isneg, "-123");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 123LU, "%" PRIu64);
  test_assert_equal(isneg, 1, "%d");

  ret = try_parse_u64_neg(&res, &isneg, "18446744073709551615");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 18446744073709551615UL, "%" PRIu64);
  test_assert_equal(isneg, 0, "%d");

  ret = try_parse_u64_neg(&res, &isneg, "-18446744073709551615");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 18446744073709551615UL, "%" PRIu64);
  test_assert_equal(isneg, 1, "%d");

  ret = try_parse_u64_neg(&res, &isneg, "18446744073709551616");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u64_neg(&res, &isneg, "abc");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_u64_neg(&res, &isneg, "-abc");
  test_assert_equal(ret, -1, "%d");
}

int try_parse_cf128(cf128 *dest, const char *str) {
  double real, imag;
  char sign;

  int matched = sscanf(str, "%lf %c i%lf", &real, &sign, &imag);

  if (matched == 3 && (sign == '+' || sign == '-')) {
    if (sign == '-') {
      imag = -imag;
    }
    *dest = real + imag * I;
    return 0;
  }

  return -1;
}

TEST(cf128_parse) {
  cf128 res;
  int ret;

  ret = try_parse_cf128(&res, "3.0+i4.0");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(creal(res), 3.0, "%f");
  test_assert_equal(cimag(res), 4.0, "%f");

  ret = try_parse_cf128(&res, "2.5-i1.5");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(creal(res), 2.5, "%f");
  test_assert_equal(cimag(res), -1.5, "%f");

  ret = try_parse_cf128(&res, "5.0");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_cf128(&res, "i2.0");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_cf128(&res, "3.0+4.0i");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_cf128(&res, " 3.0 + i4.0 ");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(creal(res), 3.0, "%f");
  test_assert_equal(cimag(res), 4.0, "%f");

  ret = try_parse_cf128(&res, "-3.0+i2.0");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(creal(res), -3.0, "%f");
  test_assert_equal(cimag(res), 2.0, "%f");

  ret = try_parse_cf128(&res, "3.0-i2.0");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(creal(res), 3.0, "%f");
  test_assert_equal(cimag(res), -2.0, "%f");

  ret = try_parse_cf128(&res, "3.0+i0.0");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(creal(res), 3.0, "%f");
  test_assert_equal(cimag(res), 0.0, "%f");

  ret = try_parse_cf128(&res, "0.0+i0.0");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(creal(res), 0.0, "%f");
  test_assert_equal(cimag(res), 0.0, "%f");
}

int try_parse_f32_freq_str(float *dest, const char *str) {
  errno = 0;
  char *endptr;
  *dest = strtof(str, &endptr);

  if (errno == ERANGE || endptr == str)
    return -1;

  if (*endptr == '\0') {
    return 0;
  }

  char prefix[3] = "";
  if (isalpha(*endptr) && (*(endptr + 1) == 'H') && (*(endptr + 2) == 'z')) {
    prefix[0] = *endptr;
    endptr++;
  }

  if (strncmp(endptr, "Hz", 2) != 0) {
    return -1;
  }

  si_prefixf sip;
  if (si_prefixf_parse(&sip, prefix))
    return -1;

  *dest *= sip.multiplier;

  return 0;
}

TEST(try_parse_f32_freq_str) {
  float res;
  int ret;

  ret = try_parse_f32_freq_str(&res, "3.0kHz");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 3000.0f, "%f");

  ret = try_parse_f32_freq_str(&res, "1.5MHz");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 1500000.0f, "%f");

  ret = try_parse_f32_freq_str(&res, "200Hz");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 200.0f, "%f");

  ret = try_parse_f32_freq_str(&res, "5");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(res, 5.0f, "%f");

  ret = try_parse_f32_freq_str(&res, "5Gz");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_f32_freq_str(&res, "not_a_number");
  test_assert_equal(ret, -1, "%d");
}

int try_parse_f32_angle_str(float *dest, const char *str) {
  errno = 0;
  char *endptr;
  *dest = strtof(str, &endptr);

  if (errno == ERANGE || endptr == str)
    return -1;

  if (*endptr == '\0') {
    return 0;
  }

  if (strncmp(endptr, "deg", 3) == 0 && *(endptr + 3) == '\0') {
    *dest *= M_PI / 180;
    return 0;
  } else if (strncmp(endptr, "rad", 3) == 0 && *(endptr + 3) == '\0') {
    return 0;
  } else {
    return -1;
  }
}

TEST(try_parse_f32_angle_str) {
  float res;
  int ret;
  float RESOLUTION = 1e-5;

  ret = try_parse_f32_angle_str(&res, "45deg");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(fabs(res - M_PI / 4) < RESOLUTION, 1, "%d");

  ret = try_parse_f32_angle_str(&res, "3.14159rad");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(fabs(res - 3.14159f) < RESOLUTION, 1, "%d");

  ret = try_parse_f32_angle_str(&res, "90deg");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(fabs(res - M_PI / 2) < RESOLUTION, 1, "%d");

  ret = try_parse_f32_angle_str(&res, "1.5708rad");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(fabs(res - 1.5708f) < RESOLUTION, 1, "%d");

  ret = try_parse_f32_angle_str(&res, "5.5");
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(fabs(res - 5.5f) < RESOLUTION, 1, "%d");

  ret = try_parse_f32_angle_str(&res, "5grads");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_f32_angle_str(&res, "not_a_number");
  test_assert_equal(ret, -1, "%d");

  ret = try_parse_f32_angle_str(&res, "60degExtra");
  test_assert_equal(ret, -1, "%d");
}

int parse_folder_remove_trailing_slash(char *dest) {
  ASSERT(dest);
  size_t len = strlen(dest);
  if (len == 0)
    return 0;

  while (len > 1 && dest[len - 1] == '/') {
    dest[(len--) - 1] = '\0';
  }

  return 0;
}

TEST(parse_folder_remove_trailing_slash) {
  char folder1[] = "path/to/folder/";
  char folder2[] = "path/to/folder//";
  char folder3[] = "/";
  char folder4[] = "path/to/folder";
  char folder5[] = "";

  int ret;

  // Test case 1: Remove single trailing slash
  ret = parse_folder_remove_trailing_slash(folder1);
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(strcmp(folder1, "path/to/folder"), 0, "%d");

  // Test case 2: Remove multiple trailing slashes
  ret = parse_folder_remove_trailing_slash(folder2);
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(strcmp(folder2, "path/to/folder"), 0, "%d");

  // Test case 3: Root path should remain unchanged
  ret = parse_folder_remove_trailing_slash(folder3);
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(strcmp(folder3, "/"), 0, "%d");

  // Test case 4: Path without trailing slash should remain unchanged
  ret = parse_folder_remove_trailing_slash(folder4);
  test_assert_equal(ret, 0, "%d");
  test_assert_equal(strcmp(folder4, "path/to/folder"), 0, "%d");

  // Test case 5: Empty string should return an error
  ret = parse_folder_remove_trailing_slash(folder5);
  test_assert_equal(ret, 0, "%d");
}

int ramp_up_parse(ramp_up *dest, const char *str) {
  if (strcmp(str, "RU_LINEAR") == 0) {
    *dest = RU_LINEAR;
    return 0;
  }

  return -1;
}

void fname_dottop(char *dest) {
  size_t len = strlen(dest);
  size_t j = 0;

  for (size_t i = 0; i < len; ++i) {
    if (dest[i] == '.' && (i + 1 < len && isdigit(dest[i + 1]))) {
      dest[j++] = 'p';
    } else {
      dest[j++] = dest[i];
    }
  }
}

TEST(fname_dottop) {
  char name1[] = "file.1.txt";
  char name2[] = "file.123";
  char name3[] = ".1hidden";
  char name4[] = "file.txt";
  char name5[] = ".12345";
  char name6[] = "";

  fname_dottop(name1);
  test_assert_equal(strcmp(name1, "filep1.txt"), 0, "%d");

  fname_dottop(name2);
  test_assert_equal(strcmp(name2, "filep123"), 0, "%d");

  fname_dottop(name3);
  test_assert_equal(strcmp(name3, "p1hidden"), 0, "%d");

  fname_dottop(name4);
  test_assert_equal(strcmp(name4, "file.txt"), 0, "%d");

  fname_dottop(name5);
  test_assert_equal(strcmp(name5, "p12345"), 0, "%d");

  fname_dottop(name6);
  test_assert_equal(strcmp(name6, ""), 0, "%d");
}
}
