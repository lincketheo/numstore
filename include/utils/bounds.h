#pragma once

#pragma once

#include <math.h>
#include <stdint.h>

/*
 * Overflow-checking macros for fixed-width numeric types.
 * On overflow or divide-by-zero, they return false.
 *
 * Assumes operands and destination are already of correct type.
 *
 * Usage:
 *   u8 x; if (SAFE_ADD_U8(&x, a, b)) { ... }
 */

/////////////////////////////////////// UNSIGNED INT

#define SAFE_ADD_U8(dest, a, b) \
  (!__builtin_add_overflow (a, b, dest))

#define SAFE_SUB_U8(dest, a, b) \
  (!__builtin_sub_overflow (a, b, dest))

#define SAFE_MUL_U8(dest, a, b) \
  (!__builtin_mul_overflow (a, b, dest))

#define SAFE_DIV_U8(dest, a, b) \
  ((b) != 0 ? (*(dest) = (a) / (b), true) : false)

#define SAFE_ADD_U16(dest, a, b) \
  (!__builtin_add_overflow (a, b, dest))

#define SAFE_SUB_U16(dest, a, b) \
  (!__builtin_sub_overflow (a, b, dest))

#define SAFE_MUL_U16(dest, a, b) \
  (!__builtin_mul_overflow (a, b, dest))

#define SAFE_DIV_U16(dest, a, b) \
  ((b) != 0 ? (*(dest) = (a) / (b), true) : false)

#define SAFE_ADD_U32(dest, a, b) \
  (!__builtin_add_overflow (a, b, dest))

#define SAFE_SUB_U32(dest, a, b) \
  (!__builtin_sub_overflow (a, b, dest))

#define SAFE_MUL_U32(dest, a, b) \
  (!__builtin_mul_overflow (a, b, dest))

#define SAFE_DIV_U32(dest, a, b) \
  ((b) != 0 ? (*(dest) = (a) / (b), true) : false)

#define SAFE_ADD_U64(dest, a, b) \
  (!__builtin_add_overflow (a, b, dest))

#define SAFE_SUB_U64(dest, a, b) \
  (!__builtin_sub_overflow (a, b, dest))

#define SAFE_MUL_U64(dest, a, b) \
  (!__builtin_mul_overflow (a, b, dest))

#define SAFE_DIV_U64(dest, a, b) \
  ((b) != 0 ? (*(dest) = (a) / (b), true) : false)

/////////////////////////////////////// SIGNED INT

#define SAFE_ADD_I8(dest, a, b) \
  (!__builtin_add_overflow (a, b, dest))

#define SAFE_SUB_I8(dest, a, b) \
  (!__builtin_sub_overflow (a, b, dest))

#define SAFE_MUL_I8(dest, a, b) \
  (!__builtin_mul_overflow (a, b, dest))

#define SAFE_DIV_I8(dest, a, b) \
  ((b) != 0 && !((a) == I8_MIN && (b) == -1) ? (*(dest) = (a) / (b), true) : false)

#define SAFE_ADD_I16(dest, a, b) \
  (!__builtin_add_overflow (a, b, dest))

#define SAFE_SUB_I16(dest, a, b) \
  (!__builtin_sub_overflow (a, b, dest))

#define SAFE_MUL_I16(dest, a, b) \
  (!__builtin_mul_overflow (a, b, dest))

#define SAFE_DIV_I16(dest, a, b) \
  ((b) != 0 && !((a) == I16_MIN && (b) == -1) ? (*(dest) = (a) / (b), true) : false)

#define SAFE_ADD_I32(dest, a, b) \
  (!__builtin_add_overflow (a, b, dest))

#define SAFE_SUB_I32(dest, a, b) \
  (!__builtin_sub_overflow (a, b, dest))

#define SAFE_MUL_I32(dest, a, b) \
  (!__builtin_mul_overflow (a, b, dest))

#define SAFE_DIV_I32(dest, a, b) \
  ((b) != 0 && !((a) == I32_MIN && (b) == -1) ? (*(dest) = (a) / (b), true) : false)

#define SAFE_ADD_I64(dest, a, b) \
  (!__builtin_add_overflow (a, b, dest))

#define SAFE_SUB_I64(dest, a, b) \
  (!__builtin_sub_overflow (a, b, dest))

#define SAFE_MUL_I64(dest, a, b) \
  (!__builtin_mul_overflow (a, b, dest))

#define SAFE_DIV_I64(dest, a, b) \
  ((b) != 0 && !((a) == I64_MIN && (b) == -1) ? (*(dest) = (a) / (b), true) : false)

/////////////////////////////////////// FLOAT

#define SAFE_ADD_F32(dest, a, b) \
  ((*(dest) = (a) + (b)), isfinite (*(dest)))

#define SAFE_SUB_F32(dest, a, b) \
  ((*(dest) = (a) - (b)), isfinite (*(dest)))

#define SAFE_MUL_F32(dest, a, b) \
  ((*(dest) = (a) * (b)), isfinite (*(dest)))

#define SAFE_DIV_F32(dest, a, b) \
  ((b) != 0.0f ? (*(dest) = (a) / (b), isfinite (*(dest))) : false)

#define SAFE_ADD_F64(dest, a, b) \
  ((*(dest) = (a) + (b)), isfinite (*(dest)))

#define SAFE_SUB_F64(dest, a, b) \
  ((*(dest) = (a) - (b)), isfinite (*(dest)))

#define SAFE_MUL_F64(dest, a, b) \
  ((*(dest) = (a) * (b)), isfinite (*(dest)))

#define SAFE_DIV_F64(dest, a, b) \
  ((b) != 0.0 ? (*(dest) = (a) / (b), isfinite (*(dest))) : false)
