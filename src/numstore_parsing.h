#pragma once

#include <complex.h>
#include <stdint.h>

int try_parse_f32 (float *dest, const char *str);

int try_parse_f64 (double *dest, const char *str);

int try_parse_u32 (uint32_t *dest, const char *str);

int try_parse_i32 (int *dest, const char *str);

int try_parse_u64 (uint64_t *dest, const char *str);

int try_parse_u64_neg (uint64_t *dest, int *isneg, const char *str);

// <double>+i<double>
int try_parse_cf128 (complex double *dest, const char *str);
