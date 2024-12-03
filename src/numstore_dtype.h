#pragma once

#include <complex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum
{
  CHAR = 0,

  U8 = 1,
  U16 = 2,
  U32 = 3,
  U64 = 4,

  I8 = 5,
  I16 = 6,
  I32 = 7,
  I64 = 8,

  F32 = 9,
  F64 = 10,

  CF64 = 11,
  CF128 = 12,
} dtype;

static inline bool
is_dtype_valid (uint8_t d)
{
  return d <= 12;
}

typedef struct
{
  dtype v;
  int ok;
} dtype_result;

typedef struct
{
  union
  {
    char c;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    complex float cf64;
    complex float cf128;
  };
  dtype type;
} dtype_v;

typedef struct
{
  dtype_v v;
  int ok;
} dtype_v_result;

#define CASE_DTYPE_INT                                                        \
  U8:                                                                         \
  case U16:                                                                   \
  case U32:                                                                   \
  case U64:                                                                   \
  case I8:                                                                    \
  case I16:                                                                   \
  case I32:                                                                   \
  case I64

#define CASE_DTYPE_FLOAT                                                      \
  F32:                                                                        \
  case F64

#define CASE_DTYPE_COMPLEX                                                    \
  CF64:                                                                       \
  case CF128

#define CASE_1_BYTE                                                           \
  CHAR:                                                                       \
  case I8:                                                                    \
  case U8

#define CASE_2_BYTE                                                           \
  I16:                                                                        \
  case U16

#define CASE_4_BYTE                                                           \
  I32:                                                                        \
  case U32:                                                                   \
  case F32

#define CASE_8_BYTE                                                           \
  I64:                                                                        \
  case U64:                                                                   \
  case F64:                                                                   \
  case CF64

#define CASE_16_BYTE CF128

dtype_v_result strtodtype_v (const char *str, dtype type);

dtype_result strtodtype (const char *input);

int dtype_int_in_range (dtype type, uint64_t val, int isneg);

int dtype_float_in_range (dtype type, double val);

int dtype_cf_in_range (dtype type, complex double val);

void fill_buff (uint8_t *dest, size_t blen, dtype_v value);

size_t dtype_sizeof (dtype type);
