//
// Created by theo on 12/15/24.
//

#ifndef TYPES_H
#define TYPES_H

extern "C" {
#include <complex.h>
#include <float.h>
#include <stdint.h>

typedef uint8_t byte;

typedef uint8_t u8;
typedef int8_t i8;

#define U8_MAX UINT8_MAX;
#define I8_MAX INT8_MAX;

typedef uint16_t u16;
typedef int16_t i16;

#define U16_MAX UINT16_MAX;
#define I16_MAX INT16_MAX;

typedef uint32_t u32;
typedef int32_t i32;

#define U32_MAX UINT32_MAX;
#define I32_MAX INT32_MAX;

typedef uint64_t u64;
typedef int64_t i64;

#define U64_MAX UINT64_MAX;
#define I64_MAX INT64_MAX;

typedef float f32;
typedef double f64;

#define F32_MAX FLT_MAX;
#define F64_MAX DBL_MAX;

typedef _Complex float cf64;
typedef _Complex double cf128;
}

#endif // TYPES_H
