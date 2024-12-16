#include "numstore/utils/math.hpp"
#include "numstore/errors.hpp"
#include "numstore/macros.hpp"
#include "numstore/testing.hpp"
#include "numstore/types.hpp"

extern "C" {

#include <cstring>
#include <fftw3.h>
#include <float.h>
#include <limits.h>
#include <math.h>

#ifndef NOMP
#include <omp.h>
#endif

intervals fftf_to_bin(intervalf fint, float fs, size_t len) {
  ASSERT(intervalf_valid(fint));

  return (intervals){
      .s0 = freqtobin(fint.v0, fs, len),
      .s1 = freqtobin(fint.vf, fs, len),
  };
}

void fftf_zero_frange(cf64 *dest, intervalf f, float fs, size_t len) {
  zerocf_range(dest, fftf_to_bin(f, fs, len));
}

size_t next_2_power(size_t n) {
  if (n == 0)
    return 1;

  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;

  if (sizeof(size_t) > 4) {
    n |= n >> 32;
  }

  n++;

  return n;
}

TEST(nearest_power_of_two) {
  test_assert_equal(next_2_power(0), 1LU, "%zu");
  test_assert_equal(next_2_power(1), 1LU, "%zu");
  test_assert_equal(next_2_power(2), 2LU, "%zu");
  test_assert_equal(next_2_power(3), 4LU, "%zu");
  test_assert_equal(next_2_power(11), 16LU, "%zu");
}

size_t dblbuf_next_2_power(size_t cur_cap, size_t new_cap) {
  size_t twop = 1;

  while (twop * cur_cap <= new_cap) {
    twop *= 2;
  }

  return twop;
}

TEST(dblbuf_next_2_power) {
  test_assert_equal(dblbuf_next_2_power(1, 11), 16lu, "%lu");
  test_assert_equal(dblbuf_next_2_power(2, 11), 8lu, "%lu");
  test_assert_equal(dblbuf_next_2_power(3, 11), 4lu, "%lu");
  test_assert_equal(dblbuf_next_2_power(2, 16), 16lu, "%lu");
  test_assert_equal(dblbuf_next_2_power(3, 16), 8lu, "%lu");
  test_assert_equal(dblbuf_next_2_power(17, 201), 16lu, "%lu");
}

int is_power_of_2(size_t n) { return n && !(n & (n - 1)); }

TEST(is_power_of_2) {
  test_assert_equal(is_power_of_2(0), 0, "%d");
  test_assert_equal(is_power_of_2(1), 1, "%d");
  test_assert_equal(is_power_of_2(2), 1, "%d");
  test_assert_equal(is_power_of_2(4), 1, "%d");
  test_assert_equal(is_power_of_2(8), 1, "%d");
  test_assert_equal(is_power_of_2(1048576), 1, "%d");
  test_assert_equal(is_power_of_2(9), 0, "%d");
  test_assert_equal(is_power_of_2(11), 0, "%d");
  test_assert_equal(is_power_of_2(1231241), 0, "%d");
}

float cf64_power_to_f32_std(float power) { return sqrtf(power / 2.0f); }

float cf128_power_to_f64_std(double power) { return sqrt(power / 2.0); }

float signal_powerf(const cf64 *data, size_t len) {
  ASSERT(data);
  ASSERT(len > 0);

  // Kahan summation
  float ret = 0.0f;
  float correction = 0.0f;
  float mult = 1 / (float)len;

  for (size_t i = 0; i < len; ++i) {
    float y = modulus_sqf(data[i]) * mult - correction;
    float t = ret + y;
    correction = (t - ret) - y;
    ret = t;
  }

  return ret;
}

double signal_power(const cf128 *data, size_t len) {
  ASSERT(data);
  ASSERT(len > 0);

  // Kahan summation
  double ret = 0.0f;
  double correction = 0.0f;
  double mult = 1 / (double)len;

#ifndef NOMP
#pragma omp parallel for
#endif
  for (size_t i = 0; i < len; ++i) {
    double y = modulus_sq(data[i]) * mult - correction;
    double t = ret + y;
    correction = (t - ret) - y;
    ret = t;
  }

  return ret;
}

float noise_power_from_dataf(const cf64 *data_noise, const cf64 *data_no_noise,
                             size_t len) {
  ASSERT(data_noise);
  ASSERT(data_no_noise);

  // Kahan summation
  float ret = 0.0f;
  float correction = 0.0f;

#ifndef NOMP
#pragma omp parallel
#endif
  {
    float thread_ret = 0.0f;
    float thread_correction = 0.0f;

#ifndef NOMP
#pragma omp for
#endif
    for (size_t i = 0; i < len; ++i) {
      float diff = modulus_sqf(data_noise[i] - data_no_noise[i]);
      float y = diff - thread_correction;
      float t = thread_ret + y;
      thread_correction = (t - thread_ret) - y;
      thread_ret = t;
    }

#ifndef NOMP
#pragma omp critical
#endif
    {
      float y = thread_ret - correction;
      float t = ret + y;
      correction = (t - ret) - y;
      ret = t;
    }
  }

  return len > 0 ? ret / (float)len : 0.0f;
}

void scale_to_power_inl(cf64 *dest, size_t len, float power) {
  ASSERT(dest);
  ASSERT(len > 0);
  ASSERT(power >= 0);

  float _power = signal_powerf(dest, len);
  ASSERT(_power >= 0);

  float scale = 0;
  if (power > 0 && _power > 0) {
    scale = sqrtf(power / _power);
  }

#ifndef NOMP
#pragma omp parallel for
#endif
  for (size_t i = 0; i < len; ++i) {
    dest[i] *= scale;
  }
}

void gen_awgn_from_snrdb(cf64 *dest, size_t len, float sig_power, float snrdb) {
  float power = sqrtf(snrdb_to_awgn_power(snrdb, sig_power));

#ifdef RESCALE_POWERS
  gaussf_vec((float *)dest, 2 * len, 1, 0);
  scale_to_power_inl(dest, len, power);
#else
  gaussf_vec((float *)dest, 2 * len, cf128_power_to_f64_std(power), 0);
#endif
}

void gen_awgn_from_power(cf64 *dest, size_t len, float noise_power) {
#ifdef RESCALE_POWERS
  gaussf_vec((float *)dest, 2 * len, 1, 0);
  scale_to_power_inl(dest, len, noise_power);
#else
  gaussf_vec((float *)dest, 2 * len, cf64_power_to_f32_std(noise_power), 0);
#endif
}

static inline intervalf fs_interval(float fs) {
  return (intervalf){
      .v0 = -fs / 2,
      .vf = fs / 2,
  };
}

size_t freqtobin(float f, float fs, size_t len) {
  ASSERT(intervalf_f_in_exc(fs_interval(fs), f));
  return f * len / fs;
}

size_t bintofreq(size_t b, float fs, size_t len) {
  ASSERT(b < len);
  return b * fs / len;
}

void fftwf_dft_1d_one_time(size_t len, cf64 *src, cf64 *dest, int dir,
                           int type) {
  fftwf_plan match_fplan = fftwf_plan_dft_1d(len, (fftwf_complex *)src,
                                             (fftwf_complex *)dest, dir, type);
  fftwf_execute(match_fplan);
  fftwf_destroy_plan(match_fplan);
}

void cf64_build_tuner_inl(cf64 *dest, size_t len, float f_offset, float fs,
                          cf64 c0) {
  float pinc = -2.0f * M_PI * f_offset / fs;

#ifndef NOMP
#pragma omp parallel for
#endif
  for (size_t k = 0; k < len; ++k) {
    dest[k] = cexpf(I * k * pinc) * c0;
  }
}

cf64 cf64_tune_inl(cf64 *dest, size_t len, float f_offset, float fs, cf64 c0) {
  float pinc = -2.0f * M_PI * f_offset / fs;

#ifndef NOMP
#pragma omp parallel for
#endif
  for (size_t k = 0; k < len; ++k) {
    dest[k] *= cexpf(I * k * pinc) * c0;
  }

  return cexpf(I * len * pinc) * c0;
}

void cf64_shift(cf64 *sig, size_t len, float fs, float f_offset,
                float p_offset) {
  cf64 finc = cexpf(I * 2.0f * M_PI * f_offset / fs);
  cf64 fshift = cexpf(I * p_offset);

  for (size_t i = 0; i < len; ++i) {
    sig[i] *= fshift;
    fshift *= finc;
  }
}

static void linear_signal_ramp_up(cf64 *sig, size_t k0, size_t len,
                                  size_t rkf) {
  ASSERT(sig);

  // Point slope
  float y0 = 0.0f;
  float y1 = 1.0f;
  float x0 = 0.0f;
  float x1 = (float)rkf;
  float m = (y1 - y0) / (x1 - x0);

  for (size_t k = k0; (k - k0) < len && k < rkf; ++k) {
    sig[k - k0] *= (m * ((float)k - x0) + y0);
  }
}

const char *ramp_up_to_str(ramp_up r) {
  switch (r) {
  case RU_LINEAR:
    return "LINEAR";
  default:
    unreachable();
  }
}

// Ramps up signal (which is a slice from [k0, kf) (len == kf - k0) and rkf is
// the final sample to ramp up on
void signal_ramp_up(cf64 *sig, size_t k0, size_t len, size_t rkf,
                    ramp_up type) {
  switch (type) {
  case RU_LINEAR:
    linear_signal_ramp_up(sig, k0, len, rkf);
    break;
  default:
    unreachable();
  }
}

static void linear_signal_ramp_down(cf64 *sig, size_t k0, size_t len,
                                    size_t rk0, size_t klen) {
  ASSERT(sig);

  if (klen < rk0)
    return;

  // Point slope
  float y0 = 1.0f;
  float y1 = 0.0f;
  float x0 = (float)rk0;
  float x1 = (float)klen;
  float m = (y1 - y0) / (x1 - x0);

  for (size_t k = MAX(rk0, k0); (k - k0) < len; ++k) {
    sig[k - k0] *= (m * ((float)k - x0) + y0);
  }
}

// Ramps down signal (which is a slice from [k0, kf) (len == kf - k0) and rk0
// is the first sample to ramp down on
void signal_ramp_down(cf64 *sig, size_t k0, size_t len, size_t rk0, size_t klen,
                      ramp_up type) {
  switch (type) {
  case RU_LINEAR:
    linear_signal_ramp_down(sig, k0, len, rk0, klen);
    break;
  default:
    unreachable();
  }
}

void init_tuner_emat(cf64 *dest, size_t match_len, frange f, float fs) {
  ASSERT(frange_valid(f));

  size_t num_freqs = frange_size(f);

#ifndef NOMP
#pragma omp parallel for
#endif
  for (size_t i = 0; i < num_freqs; ++i) {
    cf64 *row = &dest[i * match_len];
    float fi = f.f0 + i * f.df;
    cf64_build_tuner_inl(row, match_len, fi, fs, 1.0);
  }
}

float snrdb_to_awgn_power(float snrdb, float sig_pwr) {
  return sig_pwr / (powf(10, snrdb / 10));
}

void cf64_rot_row_had(cf64 *dest, const cf64 *left, const cf64 *right,
                      size_t len, size_t rotnum) {
  ASSERT(dest);
  ASSERT(left);
  ASSERT(right);
  ASSERT(len > 0);
  ASSERT(rotnum > 0);
  ASSERT(is_power_of_2(len)); // a % len == a & (len - 1)

#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < rotnum; ++i) {
    cf64 *row = &dest[i * len];

    for (int j = 0; j < len; ++j) {
      row[j] = left[i] * right[(len + j - rotnum) & (len - 1)];
    }
  }
}

void cf64_row_had(cf64 *dest, const cf64 *left, const cf64 *right, size_t len) {
  ASSERT(dest);
  ASSERT(left);
  ASSERT(right);
  ASSERT(len > 0);

#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < len; ++i) {
    dest[i] = left[i] * right[i];
  }
}

void cf64_mat_vec_row_had(cf64 *dest, const cf64 *left_mat,
                          const cf64 *right_vec, size_t rows, size_t cols) {
  ASSERT(dest);
  ASSERT(left_mat);
  ASSERT(right_vec);
  ASSERT(rows > 0);
  ASSERT(cols > 0);

#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < rows; ++i) {
    cf64 *row = &dest[i * cols];
    for (int j = 0; j < cols; ++j) {
      row[j] = left_mat[i * cols + j] * right_vec[j];
    }
  }
}

void cf64_row_had_right_shift(cf64 *dest, const cf64 *left, const cf64 *right,
                              size_t len, long right_start) {
  ASSERT(len <= LONG_MAX);
  ASSERT(right_start > -(long)len);
  ASSERT(right_start < (long)len);
  ASSERT(dest);
  ASSERT(left);
  ASSERT(right);
  ASSERT(len > 0);
  ASSERT(is_power_of_2(len)); // a % len == a & (len - 1)

  right_start += len;

#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < len; ++i) {
    dest[i] = left[i] * right[(right_start + i) & (len - 1)];
  }
}

void cf64_row_had_mult_vec(cf64 *dest, const cf64 *vec, size_t rows,
                           size_t cols) {
  ASSERT(dest);
  ASSERT(vec);
  ASSERT(rows > 0);
  ASSERT(cols > 0);

#ifndef NOMP
#pragma omp parallel for
#endif
  for (size_t i = 0; i < rows; ++i) {
    cf64 *row = &dest[i * cols];
    for (size_t j = 0; j < cols; ++j) {
      row[j] *= vec[j];
    }
  }
}

void cf64_matmul_mat_vec_cf64abs(float *col_dest, const cf64 *mat,
                                 const cf64 *col_vec, size_t rows,
                                 size_t cols) {
  ASSERT(col_dest);
  ASSERT(mat);
  ASSERT(col_vec);
  ASSERT(rows > 0);
  ASSERT(cols > 0);

#ifndef NOMP
#pragma omp parallel for
#endif
  for (size_t i = 0; i < rows; ++i) {
    cf64 sum = 0.0f + 0.0f * I;

    for (size_t j = 0; j < cols; ++j) {
      sum += mat[i * cols + j] * col_vec[j];
    }

    col_dest[i] = cabsf(sum);
  }
}

void f64_vec_argmax(float *col, size_t len, float *dest, size_t *rdest) {
  ASSERT(col);
  ASSERT(len > 0);

  float max = -FLT_MAX;
  ssize_t argmax = -1;

#ifndef NOMP
#pragma omp parallel
  {
    float lmax = -FLT_MAX;
    ssize_t largmax = -1;

#pragma omp for nowait
    for (size_t i = 0; i < len; ++i) {
      if (col[i] > lmax) {
        lmax = col[i];
        largmax = i;
      }
    }

#pragma omp critical
    {
      if (lmax > max) {
        max = lmax;
        argmax = largmax;
      }
    }
  }
#else
  for (size_t i = 0; i < len; ++i) {
    if (col[i] > max) {
      max = col[i];
      argmax = i;
    }
  }
#endif

  ASSERT(argmax >= 0 && (size_t)argmax < len);

  if (dest)
    *dest = max;
  if (rdest)
    *rdest = argmax;
}

void cf64_mat_reverse_rows_inl(cf64 *dest, size_t rows, size_t cols) {
#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < rows; ++i) {
    cf64 *row = &dest[i * cols];
    reverse_inl(row, cols, sizeof *row);
  }
}

void cf64_mat_conj_inl(cf64 *dest, size_t rows, size_t cols) {
#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < rows; ++i) {
    cf64 *row = &dest[i * cols];
    conjf_inl(row, cols);
  }
}

cf64 cf64_vec_inner_prod(const cf64 *left, const cf64 *right, size_t len) {
  cf64 ret = 0.0f + 0.0f * I;

#ifndef NOMP
#pragma omp parallel for reduction(+ : ret)
#endif
  for (int i = 0; i < len; ++i) {
    ret += left[i] * conjf(right[i]);
  }
  return ret;
}

float cf64_abs_sum(const cf64 *arr, size_t len) {
  float ret = 0.0f;

#ifndef NOMP
#pragma omp parallel for reduction(+ : ret)
#endif
  for (int i = 0; i < len; ++i) {
    ret += crealf(arr[i] * conjf(arr[i]));
  }
  return ret;
}

void cf64_transpose_cf64_col_limit(cf64 *dest, const cf64 *src, size_t rows,
                                   size_t cols, size_t col_limit) {
#ifndef NOMP
#pragma omp parallel for
#endif
  for (size_t i = 0; i < rows; ++i) {
    for (size_t j = 0; j < col_limit; ++j) {
      dest[j * rows + i] = src[i * cols + j];
    }
  }
}

void cf64_transpose_f32_abs_sqrd_col_limit(float *dest, const cf64 *src,
                                           size_t rows, size_t cols,
                                           size_t col_limit) {
#ifndef NOMP
#pragma omp parallel for
#endif
  for (size_t i = 0; i < rows; ++i) {
    for (size_t j = 0; j < col_limit; ++j) {
      dest[j * rows + i] = modulus_sqf(src[i * cols + j]);
    }
  }
}

void conjf_inl(cf64 *data, size_t len) {
#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < len; ++i)
    data[i] = crealf(data[i]) - cimagf(data[i]) * I;
}

void conj_inl(cf128 *data, size_t len) {
#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < len; ++i)
    data[i] = creal(data[i]) - cimag(data[i]) * I;
}

#define _reverse_inl(_tdata, len, type)                                        \
  do {                                                                         \
    type *tdata = (type *)_tdata;                                              \
    int l = 0;                                                                 \
    int r = len - 1;                                                           \
    while (l < r) {                                                            \
      type temp = tdata[l];                                                    \
      tdata[l] = tdata[r];                                                     \
      tdata[r] = temp;                                                         \
      l++;                                                                     \
      r--;                                                                     \
    }                                                                          \
  } while (0)

void reverse_inl(void *data, size_t len, size_t esize) {
  switch (esize) {
  case 1:
    _reverse_inl(data, len, uint8_t);
    break;
  case 2:
    _reverse_inl(data, len, uint16_t);
    break;
  case 4:
    _reverse_inl(data, len, uint32_t);
    break;
  case 8:
    _reverse_inl(data, len, uint64_t);
    break;
  default:
    unreachable();
  }
}

#define _test_reverse_inline_type(type, fmt)                                   \
  {                                                                            \
    type data_odd[] = {5, 6, 7, 8, 9};                                         \
    type expected_odd[] = {9, 8, 7, 6, 5};                                     \
                                                                               \
    type data_even[] = {5, 6, 7, 8, 9, 10};                                    \
    type expected_even[] = {10, 9, 8, 7, 6, 5};                                \
                                                                               \
    reverse_inl(data_odd, 5, sizeof *data_odd);                                \
    reverse_inl(data_even, 6, sizeof *data_even);                              \
                                                                               \
    for (int i = 0; i < 5; ++i) {                                              \
      test_assert_equal(data_odd[i], expected_odd[i], fmt);                    \
    }                                                                          \
    for (int i = 0; i < 6; ++i) {                                              \
      test_assert_equal(data_even[i], expected_even[i], fmt);                  \
    }                                                                          \
  }

TEST(reverse_inl) {
  _test_reverse_inline_type(uint8_t, "%d");
  _test_reverse_inline_type(uint16_t, "%d");
  _test_reverse_inline_type(uint32_t, "%d");
  _test_reverse_inline_type(uint64_t, "%zu");
}

void roll_inline(void *data, int64_t rollby, size_t len, size_t esize) {
  uint8_t *bdata = (uint8_t *)data;
  size_t _rollby = rollby % len;
  reverse_inl(data, len, esize);
  reverse_inl(data, _rollby, esize);
  reverse_inl(bdata + esize * _rollby, len - _rollby, esize);
}

#define _test_roll_inl_with_type(type, fmt)                                    \
  {                                                                            \
    type data_odd[] = {5, 6, 7, 8, 9};                                         \
    type expected_odd[] = {7, 8, 9, 5, 6};                                     \
                                                                               \
    type data_even[] = {5, 6, 7, 8, 9, 10};                                    \
    type expected_even[] = {8, 9, 10, 5, 6, 7};                                \
                                                                               \
    roll_inline(data_odd, 3, 5, sizeof *data_odd);                             \
    roll_inline(data_even, 3, 6, sizeof *data_even);                           \
                                                                               \
    for (int i = 0; i < 5; ++i) {                                              \
      test_assert_equal(data_odd[i], expected_odd[i], fmt);                    \
    }                                                                          \
    for (int i = 0; i < 6; ++i) {                                              \
      test_assert_equal(data_even[i], expected_even[i], fmt);                  \
    }                                                                          \
  }

TEST(roll_inline) {
  _test_roll_inl_with_type(uint8_t, "%d");
  _test_roll_inl_with_type(uint16_t, "%d");
  _test_roll_inl_with_type(uint32_t, "%d");
  _test_roll_inl_with_type(uint64_t, "%zu");
}

void cf64_add_inl(cf64 *dest, cf64 *right, size_t len) {
#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < len; ++i) {
    dest[i] += right[i];
  }
}

void zerocf_range(cf64 *dest, intervals s) {
  ASSERT(s.s1 >= s.s0);
  if (s.s1 > s.s0)
    memset(&dest[s.s0], 0, (s.s1 - s.s0) * sizeof *dest);
}

float fvec_add(const float *dat, size_t len) {
  ASSERT(dat);
  float ret = 0;
#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < len; ++i) {
    ret += dat[i];
  }
  return ret;
}

float fvec_mean(const float *dat, size_t len) {
  ASSERT(dat);
  float ret = 0;
  const float invlen = 1.0f / (float)len;

#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < len; ++i) {
    ret += dat[i] * invlen;
  }
  return ret;
}

float fvec_mean_fsqrd(const float *dat, size_t len) {
  ASSERT(dat);
  float ret = 0;
  float invlen = 1.0f / (float)len;
#ifndef NOMP
#pragma omp parallel for
#endif
  for (int i = 0; i < len; ++i) {
    ret += dat[i] * dat[i] * invlen;
  }
  return ret;
}

void gauss_vec(double *dest, size_t len, double std, double mean) {
  ASSERT(len % 2 == 0); // Ensure even length for paired generation
  double mult = 1.0 / (double)RAND_MAX;

  for (int i = 0; i < len; i += 2) {
    double S, v1, v2, u1, u2;

    do {
      u1 = (double)rand() * mult;
      u2 = (double)rand() * mult;
      v1 = 2 * u1 - 1;
      v2 = 2 * u2 - 1;
      S = v1 * v1 + v2 * v2;
    } while (S >= 1);

    double v3 = std * sqrt((-2.0 * log(S)) / S);

    dest[i] = v1 * v3 + mean;
    dest[i + 1] = v2 * v3 + mean;
  }
}

void gaussf_vec(float *dest, size_t len, float std, float mean) {
  ASSERT(len % 2 == 0); // Ensure even length for paired generation
  float mult = 1.0f / (float)RAND_MAX;

  for (int i = 0; i < len; i += 2) {
    float S, v1, v2, u1, u2;

    do {
      u1 = (float)rand() * mult;
      u2 = (float)rand() * mult;
      v1 = 2.0f * u1 - 1.0f;
      v2 = 2.0f * u2 - 1.0f;
      S = v1 * v1 + v2 * v2;
    } while (S >= 1.0f || S == 0.0f);

    float v3 = std * sqrtf((-2.0f * logf(S)) / S);

    dest[i] = v1 * v3 + mean;
    dest[i + 1] = v2 * v3 + mean;
  }
}

void stats_fprint_friendly(FILE *ofp, stats s) {
  ASSERT(ofp);
  fprintf(ofp, "Mean: %.2f\n", s.mean);
  fprintf(ofp, "Standard Deviation: %.2f\n", s.std);
  fprintf(ofp, "Max: %.2f (Index: %zu)\n", s.max, s.argmax);
  fprintf(ofp, "Min: %.2f (Index: %zu)\n", s.min, s.argmin);
}

void stats_fprint_csv_title(FILE *ofp, const char *prefix, int end_comma) {
  fprintf(ofp, "%s_mean,%s_std,%s_max,%s_min,%s_argmax,%s_argmin", prefix,
          prefix, prefix, prefix, prefix, prefix);
  if (end_comma)
    fprintf(ofp, ",");
}

void stats_fprint_csv_row(FILE *ofp, stats s, int end_comma) {
  fprintf(ofp, "%f,%f,%f,%f,%zu,%zu", s.mean, s.std, s.max, s.min, s.argmax,
          s.argmin);
  if (end_comma)
    fprintf(ofp, ",");
}

#define PROCESS_STATS(data, len, transform, result)                            \
  do {                                                                         \
    ASSERT(data);                                                              \
    ASSERT(len > 0);                                                           \
                                                                               \
    (result).mean = 0.0;                                                       \
    (result).std = 0.0;                                                        \
    (result).max = transform(data[0]);                                         \
    (result).min = transform(data[0]);                                         \
    (result).argmax = 0;                                                       \
    (result).argmin = 0;                                                       \
                                                                               \
    double sum = 0.0;                                                          \
    double sum_correction = 0.0; /* For Kahan summation */                     \
    double variance = 0.0;                                                     \
    double variance_correction = 0.0; /* For Kahan summation */                \
                                                                               \
    /* First pass: Calculate sum, min, max, argmin, argmax */                  \
    for (size_t i = 0; i < len; i++) {                                         \
      double value = transform(data[i]);                                       \
                                                                               \
      /* Kahan summation for sum */                                            \
      double y = value - sum_correction;                                       \
      double t = sum + y;                                                      \
      sum_correction = (t - sum) - y;                                          \
      sum = t;                                                                 \
                                                                               \
      /* Update min, max, argmin, argmax */                                    \
      if (value > (result).max) {                                              \
        (result).max = value;                                                  \
        (result).argmax = i;                                                   \
      }                                                                        \
      if (value < (result).min) {                                              \
        (result).min = value;                                                  \
        (result).argmin = i;                                                   \
      }                                                                        \
    }                                                                          \
                                                                               \
    double mean = sum / len;                                                   \
    (result).mean = mean;                                                      \
                                                                               \
    /* Second pass: Compute variance */                                        \
    for (size_t i = 0; i < len; i++) {                                         \
      double value = transform(data[i]);                                       \
      double diff = value - mean;                                              \
      double squared_diff_over_len = (diff * diff) / len;                      \
                                                                               \
      /* Kahan summation for variance */                                       \
      double y = squared_diff_over_len - variance_correction;                  \
      double t = variance + y;                                                 \
      variance_correction = (t - variance) - y;                                \
      variance = t;                                                            \
    }                                                                          \
                                                                               \
    (result).std = sqrt(variance);                                             \
  } while (0)

stats stats_calculate(const float *data, size_t len) {
  stats result;
  PROCESS_STATS(data, len, (double), result);
  return result;
}

stats stats_calculate_abs_cf(const cf64 *data, size_t len) {
  stats result;
  PROCESS_STATS(data, len, cabsf, result);
  return result;
}
}
