#pragma once

// TODO - Platform differences
// ILP32, LP64, LLP64, etc.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef u16 f16;
typedef float f32;
typedef double f64;
typedef long double f128;

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

typedef u8 bool;

#define PRId8 "hhd"
#define PRId16 "hd"
#define PRId32 "d"
#define PRId64 "lld"

#define PRIu8 "hhu"
#define PRIu16 "hu"
#define PRIu32 "u"
#define PRIu64 "llu"

#define PRIx8 "hhx"
#define PRIx16 "hx"
#define PRIx32 "x"
#define PRIx64 "llx"

#define PRIo8 "hho"
#define PRIo16 "ho"
#define PRIo32 "o"
#define PRIo64 "llo"

#define NULL (void *)0
