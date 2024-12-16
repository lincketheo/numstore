#pragma once

#include "numstore/types.hpp"

extern "C" {

typedef enum {
  CHAR,

  U8,
  U16,
  U32,
  U64,

  I8,
  I16,
  I32,
  I64,

  F32,
  F64,

  CF64,
  CF128,
} dtype;

typedef struct {
  dtype v;
  int ok;
} dtype_result;

typedef struct {
  union {
    char _char;
    u8 _u8;
    u16 _u16;
    u32 _u32;
    u64 _u64;
    i8 _i8;
    i16 _i16;
    i32 _i32;
    i64 _i64;
    f32 _f32;
    f64 _f64;
    cf64 _cf64;
    cf128 _cf128;
  };
  dtype type;
} dtype_v;

typedef struct {
  dtype_v v;
  int ok;
} dtype_v_result;

dtype_v_result strtodtype_v(const char *str, dtype type);

dtype_result strtodtype(const char *input);

int dtype_int_in_range(dtype type, uint64_t val, int isneg);

int dtype_float_in_range(dtype type, double val);

int dtype_cf_in_range(dtype type, _Complex double val);

void fill_buff(uint8_t *dest, size_t blen, dtype_v value);

size_t dtype_sizeof(dtype type);
}
