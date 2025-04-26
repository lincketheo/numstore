#pragma once

#include "intf/stdlib.h"
#include "intf/types.h"

/////////////////////////////////////////// UNSIGNED ADD/SUB

static inline bool __attribute__ ((unused)) can_add_u8 (u8 a, u8 b) { return b <= U8_MAX - a; }
static inline bool __attribute__ ((unused)) can_add_u16 (u16 a, u16 b) { return b <= U16_MAX - a; }
static inline bool __attribute__ ((unused)) can_add_u32 (u32 a, u32 b) { return b <= U32_MAX - a; }
static inline bool __attribute__ ((unused)) can_add_u64 (u64 a, u64 b) { return b <= U64_MAX - a; }

static inline bool __attribute__ ((unused)) can_sub_u8 (u8 a, u8 b) { return a >= b; }
static inline bool __attribute__ ((unused)) can_sub_u16 (u16 a, u16 b) { return a >= b; }
static inline bool __attribute__ ((unused)) can_sub_u32 (u32 a, u32 b) { return a >= b; }
static inline bool __attribute__ ((unused)) can_sub_u64 (u64 a, u64 b) { return a >= b; }

/////////////////////////////////////////// UNSIGNED MUL/DIV

static inline bool __attribute__ ((unused)) can_mul_u8 (u8 a, u8 b) { return a == 0 || b <= U8_MAX / a; }
static inline bool __attribute__ ((unused)) can_mul_u16 (u16 a, u16 b) { return a == 0 || b <= U16_MAX / a; }
static inline bool __attribute__ ((unused)) can_mul_u32 (u32 a, u32 b) { return a == 0 || b <= U32_MAX / a; }
static inline bool __attribute__ ((unused)) can_mul_u64 (u64 a, u64 b) { return a == 0 || b <= U64_MAX / a; }

static inline bool __attribute__ ((unused)) can_div_u8 (__attribute__ ((unused)) u8 a, u8 b) { return b != 0; }
static inline bool __attribute__ ((unused)) can_div_u16 (__attribute__ ((unused)) u16 a, u16 b) { return b != 0; }
static inline bool __attribute__ ((unused)) can_div_u32 (__attribute__ ((unused)) u32 a, u32 b) { return b != 0; }
static inline bool __attribute__ ((unused)) can_div_u64 (__attribute__ ((unused)) u64 a, u64 b) { return b != 0; }

/////////////////////////////////////////// SIGNED ADD/SUB

#define DEFINE_SIGNED_ADD(type, TMIN, TMAX)                                   \
  static inline __attribute__ ((unused)) bool can_add_##type (type a, type b) \
  {                                                                           \
    if (b > 0)                                                                \
      return a <= TMAX - b;                                                   \
    return a >= TMIN - b;                                                     \
  }

#define DEFINE_SIGNED_SUB(type, TMIN, TMAX)                                   \
  static inline __attribute__ ((unused)) bool can_sub_##type (type a, type b) \
  {                                                                           \
    if (b > 0)                                                                \
      return a >= TMIN + b;                                                   \
    return a <= TMAX + (-b);                                                  \
  }

DEFINE_SIGNED_ADD (i8, I8_MIN, I8_MAX)
DEFINE_SIGNED_ADD (i16, I16_MIN, I16_MAX)
DEFINE_SIGNED_ADD (i32, I32_MIN, I32_MAX)
DEFINE_SIGNED_ADD (i64, I64_MIN, I64_MAX)

DEFINE_SIGNED_SUB (i8, I8_MIN, I8_MAX)
DEFINE_SIGNED_SUB (i16, I16_MIN, I16_MAX)
DEFINE_SIGNED_SUB (i32, I32_MIN, I32_MAX)
DEFINE_SIGNED_SUB (i64, I64_MIN, I64_MAX)

/////////////////////////////////////////// SIGNED MUL/DIV

static inline bool __attribute__ ((unused)) can_mul_i8 (i8 a, i8 b) { return can_mul_u8 ((u8)abs (a), (u8)abs (b)); }
static inline bool __attribute__ ((unused)) can_mul_i16 (i16 a, i16 b) { return can_mul_u16 ((u16)abs (a), (u16)abs (b)); }
static inline bool __attribute__ ((unused)) can_mul_i32 (i32 a, i32 b) { return can_mul_u32 ((u32)abs (a), (u32)abs (b)); }
static inline bool __attribute__ ((unused)) can_mul_i64 (i64 a, i64 b) { return can_mul_u64 ((u64)llabs (a), (u64)llabs (b)); }

static inline bool __attribute__ ((unused)) can_div_i8 (i8 a, i8 b) { return b != 0 && !(a == I8_MIN && b == -1); }
static inline bool __attribute__ ((unused)) can_div_i16 (i16 a, i16 b) { return b != 0 && !(a == I16_MIN && b == -1); }
static inline bool __attribute__ ((unused)) can_div_i32 (i32 a, i32 b) { return b != 0 && !(a == I32_MIN && b == -1); }
static inline bool __attribute__ ((unused)) can_div_i64 (i64 a, i64 b) { return b != 0 && !(a == I64_MIN && b == -1); }

/////////////////////////////////////////// FLOAT ADD/SUB/MUL/DIV

static inline bool __attribute__ ((unused)) can_add_f32 (f32 a, f32 b)
{
  f32 r = a + b;
  return isfinite (r) && fpclassify (r) != FP_SUBNORMAL;
}

static inline bool __attribute__ ((unused)) can_add_f64 (f64 a, f64 b)
{
  f64 r = a + b;
  return isfinite (r) && fpclassify (r) != FP_SUBNORMAL;
}

static inline bool __attribute__ ((unused)) can_sub_f32 (f32 a, f32 b)
{
  f32 r = a - b;
  return isfinite (r) && fpclassify (r) != FP_SUBNORMAL;
}

static inline bool __attribute__ ((unused)) can_sub_f64 (f64 a, f64 b)
{
  f64 r = a - b;
  return isfinite (r) && fpclassify (r) != FP_SUBNORMAL;
}

static inline bool __attribute__ ((unused)) can_mul_f32 (f32 a, f32 b)
{
  f32 r = a * b;
  return isfinite (r) && fpclassify (r) != FP_SUBNORMAL;
}

static inline bool __attribute__ ((unused)) can_mul_f64 (f64 a, f64 b)
{
  f64 r = a * b;
  return isfinite (r) && fpclassify (r) != FP_SUBNORMAL;
}

static inline bool __attribute__ ((unused)) can_div_f32 (f32 a, f32 b)
{
  return b != 0.0f && isfinite (a / b) && fpclassify (a / b) != FP_SUBNORMAL;
}

static inline bool __attribute__ ((unused)) can_div_f64 (f64 a, f64 b)
{
  return b != 0.0 && isfinite (a / b) && fpclassify (a / b) != FP_SUBNORMAL;
}
