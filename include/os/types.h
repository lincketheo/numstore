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
