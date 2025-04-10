#pragma once

#include "dev/assert.h"

#include <complex.h>
#include <stddef.h>
#include <stdint.h>

/////// DTYPES
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef u16 f16;
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

typedef u8 bool;

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
  CU128,

  CHAR,
  BOOL,
} dtype;

u64 dtype_sizeof(const dtype type);

/////// Internal Node Types
typedef u64 page_ptr;
typedef u16 offset_t;
typedef u16 keylen_t;
typedef u16 nkeys_t;

#define PAGE_SIZE 4096

/////// RANGES
typedef struct
{
  u64 start;
  u64 end;
  u64 span;
} srange;

static inline int srange_valid(const srange* s)
{
  return s->start <= s->end;
}

DEFINE_ASSERT(srange, srange)

// Copies data from [src] into [dest] using range defined by [range]
// [dnelem] is the capacity of dest - data is appended contiguously
// [snelem] is the number of elements in src
u64 srange_copy(
    u8* dest,
    u64 dnelem,
    const u8* src,
    u64 snelem,
    srange range,
    u64 size);

/////// STRINGS
typedef struct {
  char* data;
  size_t len;
} string;
