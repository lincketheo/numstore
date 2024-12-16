#pragma once

#include "numstore/utils/math.hpp" // ramp

#include <complex.h>
#include <stdlib.h>

extern "C" {

int construct_path(const char *base, const char *suffix, char *dest,
                   size_t dlen);

int try_parse_f32(float *dest, const char *str);

int try_parse_f64(double *dest, const char *str);

int try_parse_u32(uint32_t *dest, const char *str);

int try_parse_i32(int *dest, const char *str);

int try_parse_u64(uint64_t *dest, const char *str);

int try_parse_u64_neg(uint64_t *dest, int *isneg, const char *str);

// <double>+i<double>
int try_parse_cf128(_Complex double *dest, const char *str);

// 5.123kHz
int try_parse_f32_freq_str(float *dest, const char *str);

// 4.123deg or 4.123rad (default rad)
// Returns radians
int try_parse_f32_angle_str(float *dest, const char *str);

// returns 0 on success and sets dest, else 1 and dest is unaltered
int ramp_up_parse(ramp_up *dest, const char *str);

int parse_folder_remove_trailing_slash(char *dest);

// Converts all '.' to 'p' in [dest] except the last one
// assuming file extension
void fname_dottop(char *dest);
}
