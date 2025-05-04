#pragma once

/*
 * Overflow‐checking macros for fixed‐width integer types.
 * On overflow (or divide‐by‐zero), they call panic().
 *
 * Usage:
 *   u8  x8;  SAFE_ADD_U8 (&x8, a8, b8);
 *   i32 x32; SAFE_SUB_I32(&x32, a32, b32);
 *   // etc.
 */

// ---------------- unsigned ----------------

#define SAFE_ADD_U8(dest, a, b)                                    \
  do                                                               \
    {                                                              \
      if (__builtin_add_overflow ((u8)(a), (u8)(b), (u8 *)(dest))) \
        panic ();                                                  \
    }                                                              \
  while (0)

#define SAFE_SUB_U8(dest, a, b)                                    \
  do                                                               \
    {                                                              \
      if (__builtin_sub_overflow ((u8)(a), (u8)(b), (u8 *)(dest))) \
        panic ();                                                  \
    }                                                              \
  while (0)

#define SAFE_MUL_U8(dest, a, b)                                    \
  do                                                               \
    {                                                              \
      if (__builtin_mul_overflow ((u8)(a), (u8)(b), (u8 *)(dest))) \
        panic ();                                                  \
    }                                                              \
  while (0)

#define SAFE_DIV_U8(dest, a, b)  \
  do                             \
    {                            \
      if ((b) == 0)              \
        panic ();                \
      *(dest) = (u8)((a) / (b)); \
    }                            \
  while (0)

#define SAFE_ADD_U16(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_add_overflow ((u16)(a), (u16)(b), (u16 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_SUB_U16(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_sub_overflow ((u16)(a), (u16)(b), (u16 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_MUL_U16(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_mul_overflow ((u16)(a), (u16)(b), (u16 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_DIV_U16(dest, a, b)  \
  do                              \
    {                             \
      if ((b) == 0)               \
        panic ();                 \
      *(dest) = (u16)((a) / (b)); \
    }                             \
  while (0)

#define SAFE_ADD_U32(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_add_overflow ((u32)(a), (u32)(b), (u32 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_SUB_U32(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_sub_overflow ((u32)(a), (u32)(b), (u32 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_MUL_U32(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_mul_overflow ((u32)(a), (u32)(b), (u32 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_DIV_U32(dest, a, b)  \
  do                              \
    {                             \
      if ((b) == 0)               \
        panic ();                 \
      *(dest) = (u32)((a) / (b)); \
    }                             \
  while (0)

#define SAFE_ADD_U64(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_add_overflow ((u64)(a), (u64)(b), (u64 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_SUB_U64(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_sub_overflow ((u64)(a), (u64)(b), (u64 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_MUL_U64(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_mul_overflow ((u64)(a), (u64)(b), (u64 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_DIV_U64(dest, a, b)  \
  do                              \
    {                             \
      if ((b) == 0)               \
        panic ();                 \
      *(dest) = (u64)((a) / (b)); \
    }                             \
  while (0)

// ---------------- signed ----------------

#define SAFE_ADD_I8(dest, a, b)                                    \
  do                                                               \
    {                                                              \
      if (__builtin_add_overflow ((i8)(a), (i8)(b), (i8 *)(dest))) \
        panic ();                                                  \
    }                                                              \
  while (0)

#define SAFE_SUB_I8(dest, a, b)                                    \
  do                                                               \
    {                                                              \
      if (__builtin_sub_overflow ((i8)(a), (i8)(b), (i8 *)(dest))) \
        panic ();                                                  \
    }                                                              \
  while (0)

#define SAFE_MUL_I8(dest, a, b)                                    \
  do                                                               \
    {                                                              \
      if (__builtin_mul_overflow ((i8)(a), (i8)(b), (i8 *)(dest))) \
        panic ();                                                  \
    }                                                              \
  while (0)

#define SAFE_DIV_I8(dest, a, b)                   \
  do                                              \
    {                                             \
      if ((b) == 0 || (a) == I8_MIN && (b) == -1) \
        panic ();                                 \
      *(dest) = (i8)((a) / (b));                  \
    }                                             \
  while (0)

#define SAFE_ADD_I16(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_add_overflow ((i16)(a), (i16)(b), (i16 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_SUB_I16(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_sub_overflow ((i16)(a), (i16)(b), (i16 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_MUL_I16(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_mul_overflow ((i16)(a), (i16)(b), (i16 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_DIV_I16(dest, a, b)                   \
  do                                               \
    {                                              \
      if ((b) == 0 || (a) == I16_MIN && (b) == -1) \
        panic ();                                  \
      *(dest) = (i16)((a) / (b));                  \
    }                                              \
  while (0)

#define SAFE_ADD_I32(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_add_overflow ((i32)(a), (i32)(b), (i32 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_SUB_I32(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_sub_overflow ((i32)(a), (i32)(b), (i32 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_MUL_I32(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_mul_overflow ((i32)(a), (i32)(b), (i32 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_DIV_I32(dest, a, b)                   \
  do                                               \
    {                                              \
      if ((b) == 0 || (a) == I32_MIN && (b) == -1) \
        panic ();                                  \
      *(dest) = (i32)((a) / (b));                  \
    }                                              \
  while (0)

#define SAFE_ADD_I64(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_add_overflow ((i64)(a), (i64)(b), (i64 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_SUB_I64(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_sub_overflow ((i64)(a), (i64)(b), (i64 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_MUL_I64(dest, a, b)                                      \
  do                                                                  \
    {                                                                 \
      if (__builtin_mul_overflow ((i64)(a), (i64)(b), (i64 *)(dest))) \
        panic ();                                                     \
    }                                                                 \
  while (0)

#define SAFE_DIV_I64(dest, a, b)                   \
  do                                               \
    {                                              \
      if ((b) == 0 || (a) == I64_MIN && (b) == -1) \
        panic ();                                  \
      *(dest) = (i64)((a) / (b));                  \
    }                                              \
  while (0)
