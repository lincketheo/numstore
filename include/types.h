#pragma once

#include <assert.h>
#include <complex.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

/////// DTYPES
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;
typedef long double f128;

typedef complex float cf64;
typedef complex double cf128;
typedef complex long double cf256;

typedef i8 ci16[2];
typedef i16 ci32[2];
typedef i32 ci64[2];
typedef i64 ci128[2];

typedef u8 cu16[2];
typedef u16 cu32[2];
typedef u32 cu64[2];
typedef u64 cu128[2];

typedef u64 usize;
typedef long ssize;

#ifdef SIZE_T_MAX
#define USIZE_MAX SIZE_T_MAX
#else
#define USIZE_MAX SIZE_MAX
#endif

typedef enum {
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
  F128,
  CF64,
  CF128,
  CF256,
  CI16,
  CI32,
  CI64,
  CI128,
  CU16,
  CU32,
  CU64,
  CU128
} dtype;

static inline size_t
dtype_sizeof(const dtype type)
{
  switch (type) {
  case U8:
  case I8:
    return 1;
  case U16:
  case I16:
    return 2;
  case U32:
  case I32:
  case F32:
    return 4;
  case U64:
  case I64:
  case F64:
    return 8;
  case F128:
    return 16;
  case CF64:
  case CI16:
  case CU16:
    return 8;
  case CF128:
  case CI32:
  case CU32:
    return 16;
  case CF256:
  case CI64:
  case CU64:
    return 32;
  case CI128:
  case CU128:
    return 64;
  }
  return 0;
}

/////// RANGES
typedef struct
{
  usize start;
  usize end;
  usize span;
} srange;

#define srange_assert(s) assert((s)->start <= (s)->end);

// Copies data from [src] into [dest] using range defined by [range]
// [dnelem] is the capacity of dest - data is appended contiguously
// [snelem] is the number of elements in src
usize srange_copy(u8* dest, usize dnelem, const u8* src, usize snelem,
    srange range, usize size);

//////// BYTES
typedef struct
{
  void* data;
  usize len;
} bytes;
