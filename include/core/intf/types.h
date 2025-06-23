#pragma once

////////////////////////////////// CORE TYPES
// TODO - Platform differences
// ILP32, LP64, LLP64, etc.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#define U8_MIN 0
#define U16_MIN 0
#define U32_MIN 0
#define U64_MIN 0
#define U8_MAX ((u8) ~(u8)0)
#define U16_MAX ((u16) ~(u16)0)
#define U32_MAX ((u32) ~(u32)0)
#define U64_MAX ((u64) ~(u64)0)

#define PRIu8 "hhu"
#define PRIu16 "hu"
#define PRIu32 "u"
#define PRIu64 "llu"

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// ABS_MAX is the max of abs(value)
#define I8_MAX ((i8)(U8_MAX >> 1))
#define I8_ABS_MAX ((u8)(I8_MAX) + 1)
#define I16_MAX ((i16)(U16_MAX >> 1))
#define I16_ABS_MAX ((u16)(I16_MAX) + 1)
#define I32_MAX ((i32)(U32_MAX >> 1))
#define I32_ABS_MAX ((u32)(I32_MAX) + 1)
#define I64_MAX ((i64)(U64_MAX >> 1))
#define I64_ABS_MAX ((u64)(I64_MAX) + 1)
#define I8_MIN ((i8)(~I8_MAX))
#define I16_MIN ((i16)(~I16_MAX))
#define I32_MIN ((i32)(~I32_MAX))
#define I64_MIN ((i64)(~I64_MAX))

#define PRId8 "hhd"
#define PRId16 "hd"
#define PRId32 "d"
#define PRId64 "lld"

typedef u16 f16;
typedef float f32;
typedef double f64;
typedef long double f128;

typedef f16 cf32[2];
typedef float _Complex cf64;
typedef double _Complex cf128;
typedef long double _Complex cf256;

typedef i8 ci16[2];
typedef i16 ci32[2];
typedef i32 ci64[2];
typedef i64 ci128[2];

typedef u8 cu16[2];
typedef u16 cu32[2];
typedef u32 cu64[2];
typedef u64 cu128[2];

typedef int bool;
#define true 1
#define false 0

////////////////////////////////// DOMAIN TYPES
/**
 * Indexing into an array uses rational number
 * to support overlaps
 */
typedef u32 t_size;  // Represents the size of a single type in bytes
typedef u32 p_size;  // To index inside a page
typedef u64 b_size;  // Bytes size to index into a contiguous rope bytes
typedef i64 sb_size; // Signed b size
typedef u64 pgno;    // Page number
typedef i64 spgno;   // Page number
typedef u8 pgh;      // Page header

#define PRt_size PRIu32
#define PRp_size PRIu32
#define PRb_size PRId64
#define PRpgno PRIu64
#define PRpgh PRIu8

#ifndef NULL
#define NULL (void *)0
#endif
