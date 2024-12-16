#include "numstore/datastruct.hpp"

extern "C" {
#include <complex.h>

#define cf_unwrap(c) creal(c), cimag(c)

///////////////////////////////////////////////////////
///////// Math

static inline double modulus_sq(const cf128 z) {
  return (creal(z) * creal(z) + cimag(z) * cimag(z));
}

static inline float modulus_sqf(const cf64 z) {
  return (crealf(z) * crealf(z) + cimagf(z) * cimagf(z));
}

static inline double modulus(const cf128 z) { return (sqrt(modulus_sq(z))); }

static inline float modulusf(const cf64 z) { return (sqrtf(modulus_sqf(z))); }

intervals fftf_to_bin(intervalf fint, float fs, size_t len);

void fftf_zero_frange(cf64 *dest, intervalf fint, float fs, size_t len);

size_t next_2_power(size_t n);

size_t dblbuf_next_2_power(size_t cur, size_t newlen);

int is_power_of_2(size_t s);

float cf64_power_to_f32_std(float power);

float signal_powerf(const cf64 *data, size_t len);

float noise_power_from_dataf(const cf64 *data_noise, const cf64 *data_no_noise,
                             size_t len);

double signal_power(const cf128 *data, size_t len);

void scale_to_power_inl(cf64 *dest, size_t len, float power);

void gen_awgn_from_snrdb(cf64 *dest, size_t len, float sig_power, float snrdb);

void gen_awgn_from_power(cf64 *dest, size_t len, float noise_power);

size_t freqtobin(float f, float fs, size_t len);

size_t bintofreq(size_t b, float fs, size_t len);

void fftwf_dft_1d_one_time(size_t len, cf64 *src, cf64 *dest, int dir,
                           int type);

/**
 * Creates a 2d matrix (assumes dest is the correct size)
 * Each row is e^(i * (2pi * f * k/fs)) where k is the column number
 * and f is the frequency bin from f.f0 to f.f1
 *
 * rows: frange_size(f)
 * cols: slen
 */
void init_tuner_emat(cf64 *dest, size_t slen, frange f, float fs);

void cf64_build_tuner_inl(cf64 *dest, size_t len, float f_offset, float fs,
                          cf64 c0);

/**
 * Tunes a cf64 array by f_offset with sample frequency fs
 * Starts tuner at phasor. Usually phasor should be 1, and if you have
 * multiple tunes (multiple buffers) with continuous phase,
 * you keep passing phasor to the function
 */
cf64 cf64_tune_inl(cf64 *dest, size_t len, float f_offset, float fs,
                   cf64 phasor);

float snrdb_to_awgn_power(float snrdb, float sig_pwr);

void cf64_shift(cf64 *sig, size_t len, float fs, float f_offset,
                float p_offset);

typedef enum {
  RU_LINEAR,
} ramp_up;

const char *ramp_up_to_str(ramp_up r);

// Ramps up signal (which is a slice from [k0, kf) (len == kf - k0) and rkf is
// the final sample to ramp up on
void signal_ramp_up(cf64 *sig, size_t k0, size_t len, size_t rkf, ramp_up type);

// Ramps down signal (which is a slice from [k0, kf) (len == kf - k0) and rk0
// is the first sample to ramp down on. Also need total signal length (klen)
void signal_ramp_down(cf64 *sig, size_t k0, size_t len, size_t rk0, size_t klen,
                      ramp_up type);

///////////////////////////////////////////////////////
///////// Matrix Math

// left: vector (len)
// right: vector (len)
// dest: matrix (rotnum x len)
//
// dest[i] = left (o) roll(right, i)
void cf64_rot_row_had(cf64 *dest, const cf64 *left, const cf64 *right,
                      size_t len, size_t rotnum);

// left: vector (len)
// right: vector (len)
// dest: vector (len)
//
// dest = left (o) right
void cf64_row_had(cf64 *dest, const cf64 *left, const cf64 *right, size_t len);

// left: matrix (rows x cols)
// right: vector (cols)
// dest: vector (rows x cols)
//
// dest[i] = left[i] (o) right
void cf64_mat_vec_row_had(cf64 *dest, const cf64 *left_mat,
                          const cf64 *right_vec, size_t rows, size_t cols);

// left: vector (len)
// right: vector (len)
// dest: vector (len)
//
// dest = left (o) roll(right, right_start)
void cf64_row_had_right_shift(cf64 *dest, const cf64 *left, const cf64 *right,
                              size_t len, long right_start);

// dest: matrix (rows x cols)
// vec: vector (cols)
//
// dest[i] = dest[i] (o) vec
void cf64_row_had_mult_vec(cf64 *dest, const cf64 *vec, size_t rows,
                           size_t cols);

// col_dest: vector (rows)
// mat: matrix (rows x cols)
// col_vec: vector (cols)
//
// col_dest = cabsf(mat x col_vec)
void cf64_matmul_mat_vec_cf64abs(float *col_dest, const cf64 *mat,
                                 const cf64 *col_vec, size_t rows, size_t cols);

void cf64_mat_reverse_rows_inl(cf64 *dest, size_t rows, size_t cols);

void cf64_mat_conj_inl(cf64 *dest, size_t rows, size_t cols);

void f64_vec_argmax(float *col, size_t len, float *dest, size_t *rdest);

cf64 cf64_vec_inner_prod(const cf64 *left, const cf64 *right, size_t len);

float cf64_abs_sum(const cf64 *arr, size_t len);

void cf64_transpose_cf64_col_limit(cf64 *dest, const cf64 *src, size_t rows,
                                   size_t cols, size_t col_limit);

void cf64_transpose_f32_abs_sqrd_col_limit(float *dest, const cf64 *src,
                                           size_t rows, size_t cols,
                                           size_t col_limit);

void conjf_inl(cf64 *data, size_t len);

void conj_inl(cf128 *data, size_t len);

void reverse_inl(void *data, size_t len, size_t esize);

void roll_inline(void *data, int64_t rollby, size_t len, size_t esize);

void cf64_add_inl(cf64 *dest, cf64 *right, size_t len);

void zerocf_range(cf64 *dest, intervals s);

float fvec_add(const float *dat, size_t len);

float fvec_mean(const float *dat, size_t len);

float fvec_mean_fsqrd(const float *dat, size_t len);

///////////////////////////////////////////////////////
///////// Random

void gaussf_vec(float *dest, size_t len, float std, float mean);

void gauss_vec(double *dest, size_t len, double std, double mean);

///////////////////////////////////////////////////////
///////// Stats

typedef struct {
  double mean;
  double std;
  double max;
  double min;
  size_t argmax;
  size_t argmin;
} stats;

void stats_fprint_friendly(FILE *ofp, stats s);

void stats_fprint_csv_title(FILE *ofp, const char *prefix, int end_comma);

void stats_fprint_csv_row(FILE *ofp, stats s, int end_comma);

stats stats_calculate(const float *data, size_t len);

stats stats_calculate_abs_cf(const cf64 *data, size_t len);
}
